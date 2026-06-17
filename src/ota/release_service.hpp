#pragma once

#include <map>
#include <string>
#include <vector>

#include <httplib.h>
#include <nlohmann/json.hpp>

namespace ota {

struct SignedReleaseManifest {
  std::string channel{"dev"};
  std::string version;
  std::string artifact_url;
  std::string sha256;
  std::string sig_ed25519;
  int rollout_percent{100};
  std::string min_version;
  std::string rollback_protection;
  std::vector<std::string> revoked_devices;
};

struct EligibilityDecision {
  bool eligible{false};
  std::string reason;
  SignedReleaseManifest manifest;
};

void to_json(nlohmann::json& j, const SignedReleaseManifest& manifest);
void from_json(const nlohmann::json& j, SignedReleaseManifest& manifest);
void to_json(nlohmann::json& j, const EligibilityDecision& decision);

class ReleaseService {
 public:
  bool publish(SignedReleaseManifest manifest);
  bool promote(const std::string& version, const std::string& from_channel, const std::string& to_channel);
  EligibilityDecision eligible(const std::string& device_id, const std::string& channel,
                               const std::string& current_version) const;
  const std::vector<nlohmann::json>& audit_log() const { return audit_log_; }
  void register_routes(httplib::Server& server, const std::string& bearer_token = "");

 private:
  void audit(const EligibilityDecision& decision) const;
  mutable std::vector<nlohmann::json> audit_log_;
  std::map<std::pair<std::string, std::string>, SignedReleaseManifest> releases_;
};

}  // namespace ota
