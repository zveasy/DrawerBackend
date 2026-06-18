#include "ota/release_service.hpp"

#include <algorithm>

#include "obs/metrics.hpp"
#include "ops/operational_status.hpp"
#include "ota/agent.hpp"

namespace ota {
namespace {

bool supported_channel(const std::string& channel) {
  return channel == "dev" || channel == "pilot" || channel == "stable";
}

bool can_promote(const std::string& from, const std::string& to) {
  return (from == "dev" && to == "pilot") || (from == "pilot" && to == "stable");
}

bool contains(const std::vector<std::string>& xs, const std::string& value) {
  for (const auto& x : xs) {
    if (x == value) return true;
  }
  return false;
}

bool authorized(const httplib::Request& req, const std::string& token) {
  return token.empty() || req.get_header_value("Authorization") == "Bearer " + token;
}

void reject(httplib::Response& res, int status, const std::string& reason) {
  res.status = status;
  res.set_content(nlohmann::json{{"ok", false}, {"reason", reason}}.dump(), "application/json");
}

}  // namespace

void to_json(nlohmann::json& j, const SignedReleaseManifest& manifest) {
  j = {{"channel", manifest.channel},
       {"version", manifest.version},
       {"artifact_url", manifest.artifact_url},
       {"sha256", manifest.sha256},
       {"sig_ed25519", manifest.sig_ed25519},
       {"rollout_percent", manifest.rollout_percent},
       {"min_version", manifest.min_version},
       {"rollback_protection", manifest.rollback_protection},
       {"revoked_devices", manifest.revoked_devices}};
}

void from_json(const nlohmann::json& j, SignedReleaseManifest& manifest) {
  manifest.channel = j.value("channel", manifest.channel);
  manifest.version = j.value("version", manifest.version);
  manifest.artifact_url = j.value("artifact_url", manifest.artifact_url);
  manifest.sha256 = j.value("sha256", manifest.sha256);
  manifest.sig_ed25519 = j.value("sig_ed25519", manifest.sig_ed25519);
  manifest.rollout_percent = j.value("rollout_percent", manifest.rollout_percent);
  manifest.min_version = j.value("min_version", manifest.min_version);
  manifest.rollback_protection = j.value("rollback_protection", manifest.rollback_protection);
  manifest.revoked_devices = j.value("revoked_devices", manifest.revoked_devices);
}

void to_json(nlohmann::json& j, const EligibilityDecision& decision) {
  j = {{"eligible", decision.eligible}, {"reason", decision.reason}, {"manifest", decision.manifest}};
}

bool ReleaseService::publish(SignedReleaseManifest manifest) {
  if (!supported_channel(manifest.channel) || manifest.version.empty() || manifest.sha256.empty()) return false;
  manifest.rollout_percent = std::max(0, std::min(100, manifest.rollout_percent));
  releases_[{manifest.channel, manifest.version}] = std::move(manifest);
  return true;
}

bool ReleaseService::promote(const std::string& version, const std::string& from_channel,
                             const std::string& to_channel) {
  if (!can_promote(from_channel, to_channel)) return false;
  auto it = releases_.find({from_channel, version});
  if (it == releases_.end()) return false;
  auto promoted = it->second;
  promoted.channel = to_channel;
  return publish(promoted);
}

EligibilityDecision ReleaseService::eligible(const std::string& device_id, const std::string& channel,
                                             const std::string& current_version) const {
  EligibilityDecision decision;
  for (const auto& item : releases_) {
    const auto& manifest = item.second;
    if (manifest.channel != channel) continue;
    if (decision.manifest.version.empty() || manifest.version > decision.manifest.version) {
      decision.manifest = manifest;
    }
  }
  if (decision.manifest.version.empty()) {
    decision.reason = "no_release";
  } else if (contains(decision.manifest.revoked_devices, device_id)) {
    decision.reason = "revoked";
  } else if (decision.manifest.version <= current_version) {
    decision.reason = "stale_version";
  } else if (!decision.manifest.min_version.empty() && current_version < decision.manifest.min_version) {
    decision.reason = "rollback_protection";
  } else if (!Agent::allow(Agent::hash_device(device_id), decision.manifest.rollout_percent)) {
    decision.reason = "rollout";
  } else {
    decision.eligible = true;
    decision.reason = "eligible";
  }
  audit(decision);
  if (!decision.eligible) {
    obs::M().counter("register_ota_rejects_total", "OTA eligibility rejects",
                     {{"reason", decision.reason}})
        .inc();
    ops::update_ota(decision.reason);
  } else {
    ops::update_ota("eligible");
  }
  return decision;
}

void ReleaseService::register_routes(httplib::Server& server, const std::string& bearer_token) {
  server.Get(R"(/ota/v1/releases/([^/]+)/eligible)",
             [this, bearer_token](const httplib::Request& req, httplib::Response& res) {
               if (!authorized(req, bearer_token)) return reject(res, 401, "unauthorized");
               auto device_id = req.get_param_value("device_id");
               auto current = req.get_param_value("current_version");
               auto decision = eligible(device_id, req.matches[1], current);
               res.status = decision.eligible ? 200 : 409;
               res.set_content(nlohmann::json(decision).dump(), "application/json");
             });
  server.Post("/ota/v1/releases", [this, bearer_token](const httplib::Request& req, httplib::Response& res) {
    if (!authorized(req, bearer_token)) return reject(res, 401, "unauthorized");
    try {
      auto manifest = nlohmann::json::parse(req.body).get<SignedReleaseManifest>();
      bool ok = publish(manifest);
      res.status = ok ? 200 : 400;
      res.set_content(nlohmann::json{{"ok", ok}, {"reason", ok ? "" : "invalid_manifest"}}.dump(),
                      "application/json");
    } catch (...) {
      reject(res, 400, "malformed_payload");
    }
  });
  server.Post("/ota/v1/promote", [this, bearer_token](const httplib::Request& req, httplib::Response& res) {
    if (!authorized(req, bearer_token)) return reject(res, 401, "unauthorized");
    try {
      auto j = nlohmann::json::parse(req.body);
      bool ok = promote(j.value("version", ""), j.value("from_channel", ""), j.value("to_channel", ""));
      res.status = ok ? 200 : 400;
      res.set_content(nlohmann::json{{"ok", ok}, {"reason", ok ? "" : "invalid_promotion"}}.dump(),
                      "application/json");
    } catch (...) {
      reject(res, 400, "malformed_payload");
    }
  });
}

void ReleaseService::audit(const EligibilityDecision& decision) const {
  audit_log_.push_back({{"event", "ota_eligibility"},
                        {"eligible", decision.eligible},
                        {"reason", decision.reason},
                        {"channel", decision.manifest.channel},
                        {"version", decision.manifest.version}});
}

}  // namespace ota
