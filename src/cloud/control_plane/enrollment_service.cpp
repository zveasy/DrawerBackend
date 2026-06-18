#include "cloud/control_plane/enrollment_service.hpp"

#include <chrono>
#include <filesystem>
#include <fstream>

#include "cloud/fleet_manager/fleet_manager.hpp"
#include "obs/metrics.hpp"
#include "ops/operational_status.hpp"

namespace cloud::control_plane {
namespace {

bool malformed_token(const std::string& token) {
  if (token.size() < 12 || token.size() > 256) return true;
  for (char c : token) {
    bool ok = (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') ||
              c == '-' || c == '_' || c == '.';
    if (!ok) return true;
  }
  return false;
}

long now_epoch() {
  return static_cast<long>(std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()));
}

bool authorized(const httplib::Request& req, const std::string& token) {
  return token.empty() || req.get_header_value("Authorization") == "Bearer " + token;
}

void reject(httplib::Response& res, int status, const std::string& reason) {
  res.status = status;
  res.set_content(nlohmann::json{{"ok", false}, {"reason", reason}}.dump(), "application/json");
}

}  // namespace

void to_json(nlohmann::json& j, const EnrollmentTokenRecord& record) {
  j = {{"token", record.token},
       {"device_id", record.device_id},
       {"merchant_id", record.merchant_id},
       {"region", record.region},
       {"environment", record.environment},
       {"deployment_channel", record.deployment_channel},
       {"expires_at_epoch", record.expires_at_epoch},
       {"used", record.used},
       {"revoked", record.revoked}};
}

void from_json(const nlohmann::json& j, EnrollmentTokenRecord& record) {
  record.token = j.value("token", record.token);
  record.device_id = j.value("device_id", record.device_id);
  record.merchant_id = j.value("merchant_id", record.merchant_id);
  record.region = j.value("region", record.region);
  record.environment = j.value("environment", record.environment);
  record.deployment_channel = j.value("deployment_channel", record.deployment_channel);
  record.expires_at_epoch = j.value("expires_at_epoch", record.expires_at_epoch);
  record.used = j.value("used", record.used);
  record.revoked = j.value("revoked", record.revoked);
}

EnrollmentService::EnrollmentService(std::string store_path) : store_path_(std::move(store_path)) {
  load();
}

void EnrollmentService::add_token(EnrollmentTokenRecord record) {
  std::lock_guard<std::mutex> lk(mu_);
  tokens_[record.token] = std::move(record);
  save_locked();
}

void EnrollmentService::revoke_token(const std::string& token) {
  std::lock_guard<std::mutex> lk(mu_);
  auto it = tokens_.find(token);
  if (it != tokens_.end()) it->second.revoked = true;
  save_locked();
}

EnrollmentResult EnrollmentService::enroll(const EnrollmentRequest& request, long now) {
  EnrollmentResult result;
  if (malformed_token(request.enrollment_token)) {
    result.reason = "malformed_token";
  } else {
    std::lock_guard<std::mutex> lk(mu_);
    auto it = tokens_.find(request.enrollment_token);
    if (it == tokens_.end()) {
      result.reason = "unknown_token";
    } else if (it->second.revoked) {
      result.reason = "revoked_token";
    } else if (it->second.used) {
      result.reason = "reused_token";
    } else if (it->second.expires_at_epoch > 0 && it->second.expires_at_epoch < now) {
      result.reason = "expired_token";
    } else if (it->second.device_id != request.device_id) {
      result.reason = "device_mismatch";
    } else {
      auto twin = fleet_manager::make_local_default_twin();
      twin.drawer_id = request.device_id;
      twin.device_id = request.device_id;
      twin.merchant_id = request.merchant_id.empty() ? it->second.merchant_id : request.merchant_id;
      twin.region = request.region.empty() ? it->second.region : request.region;
      twin.environment = request.environment.empty() ? it->second.environment : request.environment;
      twin.deployment_channel =
          request.deployment_channel.empty() ? it->second.deployment_channel : request.deployment_channel;
      twin.enrolled = true;
      twin.enrollment_state = "enrolled";
      twin.sync_status = "synced";
      twin.revision = 1;
      it->second.used = true;
      identities_[twin.device_id] = twin;
      result = {true, false, "", twin};
      save_locked();
    }
  }
  if (!result.ok) {
    obs::M().counter("register_enrollment_failures_total", "Enrollment failures",
                     {{"reason", result.reason}})
        .inc();
    ops::update_enrollment(result.reason);
  } else {
    ops::update_enrollment("enrolled");
  }
  return result;
}

std::optional<device_twin::DrawerTwin> EnrollmentService::identity(const std::string& device_id) const {
  std::lock_guard<std::mutex> lk(mu_);
  auto it = identities_.find(device_id);
  if (it == identities_.end()) return std::nullopt;
  return it->second;
}

bool EnrollmentService::is_disabled(const std::string& device_id) const {
  std::lock_guard<std::mutex> lk(mu_);
  auto it = identities_.find(device_id);
  return it != identities_.end() && it->second.disabled;
}

void EnrollmentService::disable_device(const std::string& device_id) {
  std::lock_guard<std::mutex> lk(mu_);
  auto it = identities_.find(device_id);
  if (it != identities_.end()) {
    it->second.disabled = true;
    it->second.sync_status = "disabled";
  }
  save_locked();
}

void EnrollmentService::register_routes(httplib::Server& server, const std::string& bearer_token) {
  server.Post("/control-plane/v1/enroll", [this, bearer_token](const httplib::Request& req,
                                                               httplib::Response& res) {
    if (!authorized(req, bearer_token)) return reject(res, 401, "unauthorized");
    try {
      auto j = nlohmann::json::parse(req.body);
      EnrollmentRequest request{j.value("device_id", ""),
                                j.value("merchant_id", ""),
                                j.value("region", ""),
                                j.value("environment", ""),
                                j.value("deployment_channel", ""),
                                j.value("enrollment_token", "")};
      auto result = enroll(request, now_epoch());
      res.status = result.ok ? 200 : 400;
      if (result.disabled) res.status = 423;
      res.set_content(nlohmann::json{{"ok", result.ok}, {"reason", result.reason}, {"twin", result.twin}}.dump(),
                      "application/json");
    } catch (...) {
      reject(res, 400, "malformed_payload");
    }
  });
  server.Get(R"(/control-plane/v1/devices/([^/]+)/status)",
             [this, bearer_token](const httplib::Request& req, httplib::Response& res) {
               if (!authorized(req, bearer_token)) return reject(res, 401, "unauthorized");
               bool disabled = is_disabled(req.matches[1]);
               res.status = disabled ? 423 : 200;
               res.set_content(nlohmann::json{{"disabled", disabled}}.dump(), "application/json");
             });
}

void EnrollmentService::load() {
  if (store_path_.empty()) return;
  std::ifstream in(store_path_);
  if (!in) return;
  try {
    nlohmann::json j;
    in >> j;
    for (const auto& item : j.value("tokens", nlohmann::json::array())) {
      auto rec = item.get<EnrollmentTokenRecord>();
      tokens_[rec.token] = rec;
    }
    for (const auto& item : j.value("identities", nlohmann::json::array())) {
      auto twin = item.get<device_twin::DrawerTwin>();
      identities_[twin.device_id] = twin;
    }
  } catch (...) {
  }
}

void EnrollmentService::save_locked() const {
  if (store_path_.empty()) return;
  auto parent = std::filesystem::path(store_path_).parent_path();
  if (!parent.empty()) std::filesystem::create_directories(parent);
  nlohmann::json tokens = nlohmann::json::array();
  for (const auto& item : tokens_) tokens.push_back(item.second);
  nlohmann::json identities = nlohmann::json::array();
  for (const auto& item : identities_) identities.push_back(item.second);
  std::ofstream(store_path_, std::ios::trunc)
      << nlohmann::json{{"schema", 1}, {"tokens", tokens}, {"identities", identities}}.dump(2) << "\n";
}

}  // namespace cloud::control_plane
