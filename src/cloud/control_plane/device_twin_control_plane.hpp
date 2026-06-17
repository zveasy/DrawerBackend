#pragma once

#include <optional>
#include <string>

#include "cloud/device_twin/models.hpp"

namespace cloud::control_plane {

struct SyncResult {
  bool ok{false};
  bool conflict{false};
  bool disabled{false};
  std::string reason;
  device_twin::DrawerTwin twin;
};

struct EnrollmentRequest {
  std::string device_id;
  std::string merchant_id;
  std::string region;
  std::string environment;
  std::string deployment_channel;
  std::string enrollment_token;
};

struct EnrollmentResult {
  bool ok{false};
  bool disabled{false};
  std::string reason;
  device_twin::DrawerTwin twin;
};

class DeviceTwinControlPlane {
 public:
  virtual ~DeviceTwinControlPlane() = default;
  virtual EnrollmentResult enroll(const EnrollmentRequest& request) = 0;
  virtual SyncResult push_twin(const device_twin::DrawerTwin& twin) = 0;
  virtual std::optional<device_twin::DrawerTwin> fetch_twin(const std::string& drawer_id) = 0;
  virtual bool is_device_disabled(const std::string& device_id) = 0;
};

}  // namespace cloud::control_plane
