#include "edge_platform/driver.hpp"

#include <algorithm>

namespace edge_platform {

MockEdgeDriver::MockEdgeDriver(std::string device_type, std::vector<std::string> capabilities)
    : device_type_(std::move(device_type)), capabilities_(std::move(capabilities)) {}

DriverMetadata MockEdgeDriver::metadata() const {
  return {"mock-" + device_type_, "mock", device_type_, "1.0.0",
          healthy_ ? "healthy" : "faulted", capabilities_};
}

EdgeCommandResult MockEdgeDriver::execute(const EdgeDevice& device, const EdgeCommandRequest& request) {
  EdgeCommandResult result;
  result.command_id = request.command_id;
  result.device_id = device.device_id;
  result.command = request.command;
  result.completed_at = request.requested_at;
  if (!healthy_) {
    result.status = "failed";
    result.reason = "driver_unhealthy";
    return result;
  }
  result.accepted = true;
  result.executed = true;
  result.status = "executed";
  result.reason = "mock_driver_executed";
  return result;
}

std::vector<std::string> default_capabilities_for_type(const std::string& device_type) {
  if (device_type == "cash_drawer") {
    return {"inventory", "cash_out", "open_close", "lock_control", "sensor_telemetry",
            "command_execution", "firmware_update", "identity", "evidence_export"};
  }
  if (device_type == "smart_safe") {
    return {"inventory", "cash_in", "open_close", "lock_control", "sensor_telemetry",
            "command_execution", "firmware_update", "identity", "evidence_export"};
  }
  if (device_type == "cash_recycler") {
    return {"inventory", "cash_in", "cash_out", "lock_control", "sensor_telemetry",
            "command_execution", "firmware_update", "identity", "evidence_export"};
  }
  if (device_type == "teller_station") {
    return {"inventory", "cash_in", "cash_out", "open_close", "sensor_telemetry",
            "command_execution", "firmware_update", "identity", "evidence_export"};
  }
  if (device_type == "kiosk") {
    return {"inventory", "cash_in", "cash_out", "lock_control", "sensor_telemetry",
            "command_execution", "firmware_update", "identity", "evidence_export"};
  }
  if (device_type == "atm_like_terminal") {
    return {"inventory", "cash_out", "lock_control", "sensor_telemetry", "command_execution",
            "firmware_update", "identity", "evidence_export"};
  }
  return {};
}

std::unique_ptr<EdgeDriver> make_mock_driver(const std::string& device_type) {
  return std::make_unique<MockEdgeDriver>(device_type, default_capabilities_for_type(device_type));
}

}  // namespace edge_platform
