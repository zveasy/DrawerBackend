#include "cloud/control_plane/http_device_twin_client.hpp"

#include <filesystem>
#include <fstream>

#include <httplib.h>
#include <nlohmann/json.hpp>

#include "cloud/fleet_manager/fleet_manager.hpp"
#include "obs/metrics.hpp"
#include "util/log.hpp"

namespace cloud::control_plane {
namespace {

struct Endpoint {
  std::string scheme;
  std::string host;
  int port{80};
};

std::optional<Endpoint> parse_base_url(const std::string& url) {
  auto scheme_pos = url.find("://");
  if (scheme_pos == std::string::npos) return std::nullopt;
  Endpoint ep;
  ep.scheme = url.substr(0, scheme_pos);
  auto rest = url.substr(scheme_pos + 3);
  auto slash = rest.find('/');
  if (slash != std::string::npos) rest = rest.substr(0, slash);
  auto colon = rest.rfind(':');
  ep.host = colon == std::string::npos ? rest : rest.substr(0, colon);
  ep.port = ep.scheme == "https" ? 443 : 80;
  if (colon != std::string::npos) ep.port = std::stoi(rest.substr(colon + 1));
  return ep.host.empty() ? std::nullopt : std::optional<Endpoint>{ep};
}

std::unique_ptr<httplib::Client> make_client(const HttpControlPlaneConfig& config) {
  auto ep = parse_base_url(config.base_url);
  if (!ep) return {};
  auto cli = std::make_unique<httplib::Client>(ep->host, ep->port);
  auto sec = config.timeout_ms / 1000;
  auto usec = (config.timeout_ms % 1000) * 1000;
  cli->set_connection_timeout(sec, usec);
  cli->set_read_timeout(sec, usec);
  cli->set_write_timeout(sec, usec);
  return cli;
}

httplib::Headers auth_headers(const HttpControlPlaneConfig& config) {
  if (config.bearer_token.empty()) return {};
  return {{"Authorization", "Bearer " + config.bearer_token}};
}

SyncResult parse_sync_response(const httplib::Result& res) {
  SyncResult out;
  if (!res) {
    out.reason = "remote_unavailable";
    return out;
  }
  if (res->status == 401 || res->status == 403) {
    out.reason = "unauthorized";
    return out;
  }
  if (res->status == 423) {
    out.disabled = true;
    out.reason = "device_disabled";
    return out;
  }
  try {
    auto j = nlohmann::json::parse(res->body.empty() ? "{}" : res->body);
    out.reason = j.value("reason", "");
    if (j.contains("twin")) out.twin = j.at("twin").get<device_twin::DrawerTwin>();
    if (res->status == 409) {
      out.conflict = true;
      if (out.reason.empty()) out.reason = "remote_revision_conflict";
      return out;
    }
    out.ok = res->status >= 200 && res->status < 300;
    if (!out.ok && out.reason.empty()) out.reason = "http_" + std::to_string(res->status);
    return out;
  } catch (const std::exception& e) {
    out.reason = std::string("bad_response:") + e.what();
    return out;
  }
}

}  // namespace

HttpDeviceTwinClient::HttpDeviceTwinClient(HttpControlPlaneConfig config) : config_(std::move(config)) {
  status_.cloud_sync_status = "configured";
  load_queue();
  status_.offline_queue_depth = static_cast<int>(offline_queue_.size());
}

EnrollmentResult HttpDeviceTwinClient::enroll(const EnrollmentRequest& request) {
  auto cli = make_client(config_);
  if (!cli) return {false, false, "bad_control_plane_url", {}};
  nlohmann::json body = {{"device_id", request.device_id},
                         {"merchant_id", request.merchant_id},
                         {"region", request.region},
                         {"environment", request.environment},
                         {"deployment_channel", request.deployment_channel},
                         {"enrollment_token", request.enrollment_token}};
  auto res = cli->Post("/control-plane/v1/enroll", auth_headers(config_), body.dump(), "application/json");
  EnrollmentResult out;
  if (!res) {
    out.reason = "remote_unavailable";
  } else if (res->status == 423) {
    out.disabled = true;
    out.reason = "device_disabled";
  } else if (res->status >= 200 && res->status < 300) {
    try {
      auto j = nlohmann::json::parse(res->body);
      out.ok = j.value("ok", true);
      out.reason = j.value("reason", "");
      if (j.contains("twin")) out.twin = j.at("twin").get<device_twin::DrawerTwin>();
    } catch (...) {
      out.reason = "bad_response";
    }
  } else {
    try {
      out.reason = nlohmann::json::parse(res->body).value("reason", "enrollment_rejected");
    } catch (...) {
      out.reason = "http_" + std::to_string(res->status);
    }
  }
  obs::M().counter("register_enrollment_failures_total", "Enrollment failures",
                   {{"reason", out.ok ? "none" : out.reason}})
      .inc(out.ok ? 0 : 1);
  return out;
}

SyncResult HttpDeviceTwinClient::push_now(const device_twin::DrawerTwin& twin) {
  auto cli = make_client(config_);
  if (!cli) return {false, false, false, "bad_control_plane_url", twin};
  nlohmann::json body = {{"twin", twin}, {"revision", twin.revision}};
  auto res = cli->Post(("/control-plane/v1/twins/" + twin.drawer_id).c_str(), auth_headers(config_),
                       body.dump(), "application/json");
  auto out = parse_sync_response(res);
  if (out.twin.drawer_id.empty()) out.twin = twin;
  return out;
}

SyncResult HttpDeviceTwinClient::push_twin(const device_twin::DrawerTwin& twin) {
  auto out = push_now(twin);
  std::lock_guard<std::mutex> lk(mu_);
  if (out.ok) {
    status_.cloud_sync_status = "synced";
    status_.last_successful_sync_at = fleet_manager::now_iso();
    obs::M().counter("register_cloud_sync_success_total", "Cloud sync successes").inc();
  } else {
    status_.cloud_sync_status = out.conflict ? "conflict" : "sync_failed";
    status_.last_failed_sync_at = fleet_manager::now_iso();
    status_.last_error = out.reason;
    if (out.conflict) {
      obs::M().counter("register_cloud_sync_conflicts_total", "Cloud sync conflicts").inc();
    } else {
      enqueue_locked(twin, out.reason);
      obs::M().counter("register_cloud_sync_failures_total", "Cloud sync failures",
                       {{"reason", out.reason}})
          .inc();
    }
  }
  status_.offline_queue_depth = static_cast<int>(offline_queue_.size());
  return out;
}

std::optional<device_twin::DrawerTwin> HttpDeviceTwinClient::fetch_twin(const std::string& drawer_id) {
  auto cli = make_client(config_);
  if (!cli) return std::nullopt;
  auto res = cli->Get(("/control-plane/v1/twins/" + drawer_id).c_str(), auth_headers(config_));
  if (!res || res->status != 200) return std::nullopt;
  try {
    auto j = nlohmann::json::parse(res->body);
    return j.contains("twin") ? j.at("twin").get<device_twin::DrawerTwin>()
                              : j.get<device_twin::DrawerTwin>();
  } catch (...) {
    return std::nullopt;
  }
}

bool HttpDeviceTwinClient::is_device_disabled(const std::string& device_id) {
  auto cli = make_client(config_);
  if (!cli) return false;
  auto res = cli->Get(("/control-plane/v1/devices/" + device_id + "/status").c_str(),
                      auth_headers(config_));
  if (!res) return false;
  if (res->status == 423) {
    obs::M().counter("register_revoked_device_attempts_total", "Revoked device attempts").inc();
    return true;
  }
  try {
    return nlohmann::json::parse(res->body).value("disabled", false);
  } catch (...) {
    return false;
  }
}

SyncStatus HttpDeviceTwinClient::status() const {
  std::lock_guard<std::mutex> lk(mu_);
  auto out = status_;
  out.offline_queue_depth = static_cast<int>(offline_queue_.size());
  return out;
}

int HttpDeviceTwinClient::flush_offline_queue() {
  int flushed = 0;
  while (true) {
    device_twin::DrawerTwin next;
    {
      std::lock_guard<std::mutex> lk(mu_);
      if (offline_queue_.empty()) break;
      next = offline_queue_.front();
    }
    auto out = push_now(next);
    if (!out.ok) break;
    {
      std::lock_guard<std::mutex> lk(mu_);
      offline_queue_.pop_front();
      status_.cloud_sync_status = "synced";
      status_.last_successful_sync_at = fleet_manager::now_iso();
      status_.offline_queue_depth = static_cast<int>(offline_queue_.size());
      save_queue_locked();
    }
    ++flushed;
  }
  return flushed;
}

void HttpDeviceTwinClient::enqueue_locked(const device_twin::DrawerTwin& twin, const std::string& reason) {
  if (static_cast<int>(offline_queue_.size()) >= config_.max_queue_depth) offline_queue_.pop_front();
  offline_queue_.push_back(twin);
  status_.last_error = reason;
  save_queue_locked();
}

void HttpDeviceTwinClient::load_queue() {
  if (config_.offline_queue_path.empty()) return;
  std::ifstream in(config_.offline_queue_path);
  if (!in) return;
  try {
    nlohmann::json j;
    in >> j;
    for (const auto& item : j.value("twins", nlohmann::json::array())) {
      offline_queue_.push_back(item.get<device_twin::DrawerTwin>());
    }
  } catch (const std::exception& e) {
    LOG_ERROR("cloud_sync_queue_load_failed", {{"err", e.what()}});
  }
}

void HttpDeviceTwinClient::save_queue_locked() const {
  if (config_.offline_queue_path.empty()) return;
  auto parent = std::filesystem::path(config_.offline_queue_path).parent_path();
  if (!parent.empty()) std::filesystem::create_directories(parent);
  nlohmann::json twins = nlohmann::json::array();
  for (const auto& twin : offline_queue_) twins.push_back(twin);
  std::ofstream(config_.offline_queue_path, std::ios::trunc)
      << nlohmann::json{{"schema", 1}, {"twins", twins}}.dump(2) << "\n";
}

}  // namespace cloud::control_plane
