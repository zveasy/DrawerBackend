#include "ops/operational_status.hpp"

namespace ops {
namespace {
std::mutex& mu() {
  static std::mutex* m = new std::mutex();
  return *m;
}

OperationalStatus& state() {
  static OperationalStatus* s = new OperationalStatus();
  return *s;
}
}  // namespace

void update_cloud_sync(const std::string& status, int queue_depth, const std::string& ok_at,
                       const std::string& failed_at) {
  std::lock_guard<std::mutex> lk(mu());
  auto& s = state();
  s.cloud_sync_status = status;
  s.offline_queue_depth = queue_depth;
  if (!ok_at.empty()) s.last_successful_sync_at = ok_at;
  if (!failed_at.empty()) s.last_failed_sync_at = failed_at;
}

void update_enrollment(const std::string& status) {
  std::lock_guard<std::mutex> lk(mu());
  state().enrollment_status = status;
}

void update_ota(const std::string& status) {
  std::lock_guard<std::mutex> lk(mu());
  state().ota_update_status = status;
}

void update_cert_identity(const std::string& status) {
  std::lock_guard<std::mutex> lk(mu());
  state().cert_identity_status = status;
}

OperationalStatus current_status() {
  std::lock_guard<std::mutex> lk(mu());
  return state();
}

void to_json(nlohmann::json& j, const OperationalStatus& status) {
  j = {{"cloud_sync_status", status.cloud_sync_status},
       {"enrollment_status", status.enrollment_status},
       {"ota_update_status", status.ota_update_status},
       {"cert_identity_status", status.cert_identity_status},
       {"offline_queue_depth", status.offline_queue_depth},
       {"last_successful_sync_at", status.last_successful_sync_at},
       {"last_failed_sync_at", status.last_failed_sync_at}};
}

}  // namespace ops
