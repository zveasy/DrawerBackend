#pragma once

#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>

#include <httplib.h>
#include <nlohmann/json.hpp>

#include "cloud/control_plane/device_twin_control_plane.hpp"

namespace cloud::control_plane {

struct EnrollmentTokenRecord {
  std::string token;
  std::string device_id;
  std::string merchant_id;
  std::string region;
  std::string environment;
  std::string deployment_channel;
  long expires_at_epoch{0};
  bool used{false};
  bool revoked{false};
};

void to_json(nlohmann::json& j, const EnrollmentTokenRecord& record);
void from_json(const nlohmann::json& j, EnrollmentTokenRecord& record);

class EnrollmentService {
 public:
  explicit EnrollmentService(std::string store_path = "");

  void add_token(EnrollmentTokenRecord record);
  void revoke_token(const std::string& token);
  EnrollmentResult enroll(const EnrollmentRequest& request, long now_epoch);
  std::optional<device_twin::DrawerTwin> identity(const std::string& device_id) const;
  bool is_disabled(const std::string& device_id) const;
  void disable_device(const std::string& device_id);
  void register_routes(httplib::Server& server, const std::string& bearer_token = "");

 private:
  void load();
  void save_locked() const;

  mutable std::mutex mu_;
  std::string store_path_;
  std::unordered_map<std::string, EnrollmentTokenRecord> tokens_;
  std::unordered_map<std::string, device_twin::DrawerTwin> identities_;
};

}  // namespace cloud::control_plane
