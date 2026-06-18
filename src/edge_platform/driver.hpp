#pragma once

#include <memory>
#include <string>
#include <vector>

#include "edge_platform/models.hpp"

namespace edge_platform {

class EdgeDriver {
 public:
  virtual ~EdgeDriver() = default;
  virtual DriverMetadata metadata() const = 0;
  virtual EdgeCommandResult execute(const EdgeDevice& device, const EdgeCommandRequest& request) = 0;
};

class MockEdgeDriver final : public EdgeDriver {
 public:
  MockEdgeDriver(std::string device_type, std::vector<std::string> capabilities);
  DriverMetadata metadata() const override;
  EdgeCommandResult execute(const EdgeDevice& device, const EdgeCommandRequest& request) override;
  void set_healthy(bool healthy) { healthy_ = healthy; }

 private:
  std::string device_type_;
  std::vector<std::string> capabilities_;
  bool healthy_{true};
};

std::vector<std::string> default_capabilities_for_type(const std::string& device_type);
std::unique_ptr<EdgeDriver> make_mock_driver(const std::string& device_type);

}  // namespace edge_platform
