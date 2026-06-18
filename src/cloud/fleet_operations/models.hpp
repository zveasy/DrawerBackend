#pragma once

#include <map>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

namespace cloud::fleet_operations {

using Labels = std::map<std::string, std::string>;

struct Fleet {
  std::string fleet_id;
  std::string tenant_id;
  std::string name;
  std::vector<std::string> device_group_ids;
  Labels labels;
};

struct Location {
  std::string location_id;
  std::string tenant_id;
  std::string branch_id;
  std::string region;
  std::string name;
  Labels labels;
};

struct DeviceGroup {
  std::string group_id;
  std::string tenant_id;
  std::string fleet_id;
  std::string name;
  std::vector<std::string> device_ids;
  Labels labels;
};

struct DeviceEnrollment {
  std::string enrollment_id;
  std::string tenant_id;
  std::string device_id;
  std::string enrolled_at;
  std::string enrolled_by;
  std::string status{"enrolled"};
};

struct DeviceAssignment {
  std::string tenant_id;
  std::string device_id;
  std::string fleet_id;
  std::string group_id;
  std::string location_id;
  std::string branch_id;
  std::string assigned_at;
};

struct FleetDevice {
  std::string device_id;
  std::string tenant_id;
  std::string merchant_id;
  std::string branch_id;
  std::string location_id;
  std::string fleet_id;
  std::string group_id;
  std::string region;
  std::string device_type{"cash_drawer"};
  std::string firmware_version;
  std::string driver_id;
  std::string driver_version;
  std::string lifecycle_state{"provisioned"};
  std::string connectivity_status{"offline"};
  std::string last_heartbeat_at;
  long last_heartbeat_epoch{0};
  std::vector<std::string> capabilities;
  Labels labels;
  std::string quarantine_reason;
  bool retired{false};
};

struct FleetHeartbeat {
  std::string device_id;
  std::string tenant_id;
  std::string firmware_version;
  std::string driver_version;
  std::string connectivity_status{"online"};
  nlohmann::json inventory_summary = nlohmann::json::object();
  std::vector<std::string> error_codes;
  nlohmann::json health_metrics = nlohmann::json::object();
  long timestamp{0};
  std::string signature;
};

struct FleetCommand {
  std::string command_id;
  std::string tenant_id;
  std::string device_id;
  std::string command;
  nlohmann::json parameters = nlohmann::json::object();
  std::string state{"queued"};
  std::string required_capability;
  bool high_risk{false};
  std::string approval_id;
  std::string approved_by;
  std::string actor_id;
  long created_at{0};
  long expires_at{0};
  long dispatched_at{0};
  long acknowledged_at{0};
  long completed_at{0};
  std::string result;
  std::string failure_reason;
};

struct RiskFactor {
  std::string factor;
  int points{0};
  std::string explanation;
};

struct FleetRisk {
  std::string tenant_id;
  std::string scope_type{"device"};
  std::string scope_id;
  int score{0};
  std::string level{"low"};
  std::vector<RiskFactor> factors;
  long evaluated_at{0};
};

struct FleetOperationEvent {
  long sequence{0};
  std::string event_id;
  std::string tenant_id;
  std::string device_id;
  std::string event_type;
  long occurred_at{0};
  nlohmann::json payload = nlohmann::json::object();
  std::string previous_hash;
  std::string event_hash;
};

struct ReplayResult {
  bool valid{true};
  std::string reason;
  std::string hash_summary;
  std::vector<FleetOperationEvent> events;
};

void to_json(nlohmann::json& j, const Fleet& v);
void from_json(const nlohmann::json& j, Fleet& v);
void to_json(nlohmann::json& j, const Location& v);
void from_json(const nlohmann::json& j, Location& v);
void to_json(nlohmann::json& j, const DeviceGroup& v);
void from_json(const nlohmann::json& j, DeviceGroup& v);
void to_json(nlohmann::json& j, const DeviceEnrollment& v);
void from_json(const nlohmann::json& j, DeviceEnrollment& v);
void to_json(nlohmann::json& j, const DeviceAssignment& v);
void from_json(const nlohmann::json& j, DeviceAssignment& v);
void to_json(nlohmann::json& j, const FleetDevice& v);
void from_json(const nlohmann::json& j, FleetDevice& v);
void to_json(nlohmann::json& j, const FleetHeartbeat& v);
void from_json(const nlohmann::json& j, FleetHeartbeat& v);
void to_json(nlohmann::json& j, const FleetCommand& v);
void from_json(const nlohmann::json& j, FleetCommand& v);
void to_json(nlohmann::json& j, const RiskFactor& v);
void from_json(const nlohmann::json& j, RiskFactor& v);
void to_json(nlohmann::json& j, const FleetRisk& v);
void from_json(const nlohmann::json& j, FleetRisk& v);
void to_json(nlohmann::json& j, const FleetOperationEvent& v);
void from_json(const nlohmann::json& j, FleetOperationEvent& v);
void to_json(nlohmann::json& j, const ReplayResult& v);

}  // namespace cloud::fleet_operations
