#pragma once

#include <deque>
#include <mutex>
#include <string>

#include "cloud/control_plane/device_twin_control_plane.hpp"

namespace cloud::control_plane {

struct HttpControlPlaneConfig {
  std::string base_url;
  std::string bearer_token;
  std::string offline_queue_path;
  int timeout_ms{2000};
  int max_queue_depth{128};
};

class HttpDeviceTwinClient : public DeviceTwinControlPlane {
 public:
  explicit HttpDeviceTwinClient(HttpControlPlaneConfig config);

  EnrollmentResult enroll(const EnrollmentRequest& request) override;
  SyncResult push_twin(const device_twin::DrawerTwin& twin) override;
  std::optional<device_twin::DrawerTwin> fetch_twin(const std::string& drawer_id) override;
  bool is_device_disabled(const std::string& device_id) override;
  SyncStatus status() const override;
  int flush_offline_queue() override;

 private:
  SyncResult push_now(const device_twin::DrawerTwin& twin);
  void enqueue_locked(const device_twin::DrawerTwin& twin, const std::string& reason);
  void load_queue();
  void save_queue_locked() const;

  HttpControlPlaneConfig config_;
  mutable std::mutex mu_;
  SyncStatus status_;
  std::deque<device_twin::DrawerTwin> offline_queue_;
};

}  // namespace cloud::control_plane
