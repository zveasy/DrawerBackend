#pragma once

#include <mutex>
#include <string>

#include <nlohmann/json.hpp>

namespace ops {

struct OperationalStatus {
  std::string cloud_sync_status{"local"};
  std::string enrollment_status{"local"};
  std::string ota_update_status{"idle"};
  std::string cert_identity_status{"unknown"};
  int offline_queue_depth{0};
  std::string last_successful_sync_at;
  std::string last_failed_sync_at;
};

void update_cloud_sync(const std::string& status, int queue_depth, const std::string& ok_at,
                       const std::string& failed_at);
void update_enrollment(const std::string& status);
void update_ota(const std::string& status);
void update_cert_identity(const std::string& status);
OperationalStatus current_status();
void to_json(nlohmann::json& j, const OperationalStatus& status);

}  // namespace ops
