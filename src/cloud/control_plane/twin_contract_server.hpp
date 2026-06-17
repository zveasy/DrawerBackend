#pragma once

#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>

#include <httplib.h>

#include "cloud/device_twin/models.hpp"

namespace cloud::control_plane {

class TwinContractServer {
 public:
  void upsert(device_twin::DrawerTwin twin);
  std::optional<device_twin::DrawerTwin> get(const std::string& drawer_id) const;
  void disable_device(const std::string& device_id);
  void register_routes(httplib::Server& server, const std::string& bearer_token = "");

 private:
  mutable std::mutex mu_;
  std::unordered_map<std::string, device_twin::DrawerTwin> twins_;
};

}  // namespace cloud::control_plane
