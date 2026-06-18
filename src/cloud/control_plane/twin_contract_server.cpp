#include "cloud/control_plane/twin_contract_server.hpp"

#include <nlohmann/json.hpp>

#include "obs/metrics.hpp"

namespace cloud::control_plane {
namespace {
bool authorized(const httplib::Request& req, const std::string& token) {
  return token.empty() || req.get_header_value("Authorization") == "Bearer " + token;
}

void json_response(httplib::Response& res, int status, const nlohmann::json& body) {
  res.status = status;
  res.set_content(body.dump(), "application/json");
}
}  // namespace

void TwinContractServer::upsert(device_twin::DrawerTwin twin) {
  std::lock_guard<std::mutex> lk(mu_);
  twins_[twin.drawer_id] = std::move(twin);
}

std::optional<device_twin::DrawerTwin> TwinContractServer::get(const std::string& drawer_id) const {
  std::lock_guard<std::mutex> lk(mu_);
  auto it = twins_.find(drawer_id);
  if (it == twins_.end()) return std::nullopt;
  return it->second;
}

void TwinContractServer::disable_device(const std::string& device_id) {
  std::lock_guard<std::mutex> lk(mu_);
  for (auto& item : twins_) {
    if (item.second.device_id == device_id) {
      item.second.disabled = true;
      item.second.sync_status = "disabled";
    }
  }
}

void TwinContractServer::register_routes(httplib::Server& server, const std::string& bearer_token) {
  server.Post(R"(/control-plane/v1/twins/([^/]+))",
              [this, bearer_token](const httplib::Request& req, httplib::Response& res) {
                if (!authorized(req, bearer_token)) {
                  return json_response(res, 401, {{"ok", false}, {"reason", "unauthorized"}});
                }
                try {
                  auto j = nlohmann::json::parse(req.body);
                  auto incoming = j.at("twin").get<device_twin::DrawerTwin>();
                  std::lock_guard<std::mutex> lk(mu_);
                  auto it = twins_.find(req.matches[1]);
                  if (incoming.disabled || (it != twins_.end() && it->second.disabled)) {
                    obs::M().counter("register_revoked_device_attempts_total", "Revoked device attempts")
                        .inc();
                    return json_response(res, 423, {{"ok", false}, {"reason", "device_disabled"}});
                  }
                  if (it != twins_.end() && incoming.revision <= it->second.revision) {
                    obs::M().counter("register_cloud_sync_conflicts_total", "Cloud sync conflicts").inc();
                    return json_response(res, 409,
                                         {{"ok", false},
                                          {"reason", "remote_revision_conflict"},
                                          {"twin", it->second}});
                  }
                  incoming.remote_revision = incoming.revision;
                  incoming.sync_status = "synced";
                  twins_[incoming.drawer_id] = incoming;
                  json_response(res, 200, {{"ok", true}, {"twin", incoming}});
                } catch (...) {
                  json_response(res, 400, {{"ok", false}, {"reason", "malformed_payload"}});
                }
              });
  server.Get(R"(/control-plane/v1/twins/([^/]+))",
             [this, bearer_token](const httplib::Request& req, httplib::Response& res) {
               if (!authorized(req, bearer_token)) {
                 return json_response(res, 401, {{"ok", false}, {"reason", "unauthorized"}});
               }
               auto twin = get(req.matches[1]);
               if (!twin) return json_response(res, 404, {{"ok", false}, {"reason", "not_found"}});
               json_response(res, 200, {{"ok", true}, {"twin", *twin}});
             });
  server.Get(R"(/control-plane/v1/devices/([^/]+)/status)",
             [this, bearer_token](const httplib::Request& req, httplib::Response& res) {
               if (!authorized(req, bearer_token)) {
                 return json_response(res, 401, {{"ok", false}, {"reason", "unauthorized"}});
               }
               bool disabled = false;
               {
                 std::lock_guard<std::mutex> lk(mu_);
                 for (const auto& item : twins_) {
                   disabled =
                       disabled || (item.second.device_id == req.matches[1] && item.second.disabled);
                 }
               }
               json_response(res, disabled ? 423 : 200, {{"disabled", disabled}});
             });
}

}  // namespace cloud::control_plane
