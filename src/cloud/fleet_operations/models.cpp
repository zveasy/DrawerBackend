#include "cloud/fleet_operations/models.hpp"

namespace cloud::fleet_operations {

#define JSON_FIELD(name) {#name, v.name}
#define READ_FIELD(name) v.name = j.value(#name, v.name)

void to_json(nlohmann::json& j, const Fleet& v) {
  j = {JSON_FIELD(fleet_id), JSON_FIELD(tenant_id), JSON_FIELD(name),
       JSON_FIELD(device_group_ids), JSON_FIELD(labels)};
}
void from_json(const nlohmann::json& j, Fleet& v) {
  READ_FIELD(fleet_id); READ_FIELD(tenant_id); READ_FIELD(name);
  READ_FIELD(device_group_ids); READ_FIELD(labels);
}
void to_json(nlohmann::json& j, const Location& v) {
  j = {JSON_FIELD(location_id), JSON_FIELD(tenant_id), JSON_FIELD(branch_id),
       JSON_FIELD(region), JSON_FIELD(name), JSON_FIELD(labels)};
}
void from_json(const nlohmann::json& j, Location& v) {
  READ_FIELD(location_id); READ_FIELD(tenant_id); READ_FIELD(branch_id);
  READ_FIELD(region); READ_FIELD(name); READ_FIELD(labels);
}
void to_json(nlohmann::json& j, const DeviceGroup& v) {
  j = {JSON_FIELD(group_id), JSON_FIELD(tenant_id), JSON_FIELD(fleet_id),
       JSON_FIELD(name), JSON_FIELD(device_ids), JSON_FIELD(labels)};
}
void from_json(const nlohmann::json& j, DeviceGroup& v) {
  READ_FIELD(group_id); READ_FIELD(tenant_id); READ_FIELD(fleet_id);
  READ_FIELD(name); READ_FIELD(device_ids); READ_FIELD(labels);
}
void to_json(nlohmann::json& j, const DeviceEnrollment& v) {
  j = {JSON_FIELD(enrollment_id), JSON_FIELD(tenant_id), JSON_FIELD(device_id),
       JSON_FIELD(enrolled_at), JSON_FIELD(enrolled_by), JSON_FIELD(status)};
}
void from_json(const nlohmann::json& j, DeviceEnrollment& v) {
  READ_FIELD(enrollment_id); READ_FIELD(tenant_id); READ_FIELD(device_id);
  READ_FIELD(enrolled_at); READ_FIELD(enrolled_by); READ_FIELD(status);
}
void to_json(nlohmann::json& j, const DeviceAssignment& v) {
  j = {JSON_FIELD(tenant_id), JSON_FIELD(device_id), JSON_FIELD(fleet_id),
       JSON_FIELD(group_id), JSON_FIELD(location_id), JSON_FIELD(branch_id),
       JSON_FIELD(assigned_at)};
}
void from_json(const nlohmann::json& j, DeviceAssignment& v) {
  READ_FIELD(tenant_id); READ_FIELD(device_id); READ_FIELD(fleet_id);
  READ_FIELD(group_id); READ_FIELD(location_id); READ_FIELD(branch_id);
  READ_FIELD(assigned_at);
}
void to_json(nlohmann::json& j, const FleetDevice& v) {
  j = {JSON_FIELD(device_id), JSON_FIELD(tenant_id), JSON_FIELD(merchant_id),
       JSON_FIELD(branch_id), JSON_FIELD(location_id), JSON_FIELD(fleet_id),
       JSON_FIELD(group_id), JSON_FIELD(region), JSON_FIELD(device_type),
       JSON_FIELD(firmware_version), JSON_FIELD(driver_id), JSON_FIELD(driver_version),
       JSON_FIELD(lifecycle_state), JSON_FIELD(connectivity_status),
       JSON_FIELD(last_heartbeat_at), JSON_FIELD(last_heartbeat_epoch),
       JSON_FIELD(capabilities), JSON_FIELD(labels), JSON_FIELD(quarantine_reason),
       JSON_FIELD(retired)};
}
void from_json(const nlohmann::json& j, FleetDevice& v) {
  READ_FIELD(device_id); READ_FIELD(tenant_id); READ_FIELD(merchant_id);
  READ_FIELD(branch_id); READ_FIELD(location_id); READ_FIELD(fleet_id);
  READ_FIELD(group_id); READ_FIELD(region); READ_FIELD(device_type);
  READ_FIELD(firmware_version); READ_FIELD(driver_id); READ_FIELD(driver_version);
  READ_FIELD(lifecycle_state); READ_FIELD(connectivity_status);
  READ_FIELD(last_heartbeat_at); READ_FIELD(last_heartbeat_epoch);
  READ_FIELD(capabilities); READ_FIELD(labels); READ_FIELD(quarantine_reason);
  READ_FIELD(retired);
}
void to_json(nlohmann::json& j, const FleetHeartbeat& v) {
  j = {JSON_FIELD(device_id), JSON_FIELD(tenant_id), JSON_FIELD(firmware_version),
       JSON_FIELD(driver_version), JSON_FIELD(connectivity_status),
       JSON_FIELD(inventory_summary), JSON_FIELD(error_codes), JSON_FIELD(health_metrics),
       JSON_FIELD(timestamp), JSON_FIELD(signature)};
}
void from_json(const nlohmann::json& j, FleetHeartbeat& v) {
  READ_FIELD(device_id); READ_FIELD(tenant_id); READ_FIELD(firmware_version);
  READ_FIELD(driver_version); READ_FIELD(connectivity_status);
  READ_FIELD(inventory_summary); READ_FIELD(error_codes); READ_FIELD(health_metrics);
  READ_FIELD(timestamp); READ_FIELD(signature);
}
void to_json(nlohmann::json& j, const FleetCommand& v) {
  j = {JSON_FIELD(command_id), JSON_FIELD(tenant_id), JSON_FIELD(device_id),
       JSON_FIELD(command), JSON_FIELD(parameters), JSON_FIELD(state),
       JSON_FIELD(required_capability), JSON_FIELD(high_risk), JSON_FIELD(approval_id),
       JSON_FIELD(approved_by), JSON_FIELD(actor_id), JSON_FIELD(created_at),
       JSON_FIELD(expires_at), JSON_FIELD(dispatched_at), JSON_FIELD(acknowledged_at),
       JSON_FIELD(completed_at), JSON_FIELD(result), JSON_FIELD(failure_reason)};
}
void from_json(const nlohmann::json& j, FleetCommand& v) {
  READ_FIELD(command_id); READ_FIELD(tenant_id); READ_FIELD(device_id);
  READ_FIELD(command); READ_FIELD(parameters); READ_FIELD(state);
  READ_FIELD(required_capability); READ_FIELD(high_risk); READ_FIELD(approval_id);
  READ_FIELD(approved_by); READ_FIELD(actor_id); READ_FIELD(created_at);
  READ_FIELD(expires_at); READ_FIELD(dispatched_at); READ_FIELD(acknowledged_at);
  READ_FIELD(completed_at); READ_FIELD(result); READ_FIELD(failure_reason);
}
void to_json(nlohmann::json& j, const RiskFactor& v) {
  j = {JSON_FIELD(factor), JSON_FIELD(points), JSON_FIELD(explanation)};
}
void from_json(const nlohmann::json& j, RiskFactor& v) {
  READ_FIELD(factor); READ_FIELD(points); READ_FIELD(explanation);
}
void to_json(nlohmann::json& j, const FleetRisk& v) {
  j = {JSON_FIELD(tenant_id), JSON_FIELD(scope_type), JSON_FIELD(scope_id),
       JSON_FIELD(score), JSON_FIELD(level), JSON_FIELD(factors), JSON_FIELD(evaluated_at)};
}
void from_json(const nlohmann::json& j, FleetRisk& v) {
  READ_FIELD(tenant_id); READ_FIELD(scope_type); READ_FIELD(scope_id);
  READ_FIELD(score); READ_FIELD(level); READ_FIELD(factors); READ_FIELD(evaluated_at);
}
void to_json(nlohmann::json& j, const FleetOperationEvent& v) {
  j = {JSON_FIELD(sequence), JSON_FIELD(event_id), JSON_FIELD(tenant_id),
       JSON_FIELD(device_id), JSON_FIELD(event_type), JSON_FIELD(occurred_at),
       JSON_FIELD(payload), JSON_FIELD(previous_hash), JSON_FIELD(event_hash)};
}
void from_json(const nlohmann::json& j, FleetOperationEvent& v) {
  READ_FIELD(sequence); READ_FIELD(event_id); READ_FIELD(tenant_id);
  READ_FIELD(device_id); READ_FIELD(event_type); READ_FIELD(occurred_at);
  READ_FIELD(payload); READ_FIELD(previous_hash); READ_FIELD(event_hash);
}
void to_json(nlohmann::json& j, const ReplayResult& v) {
  j = {JSON_FIELD(valid), JSON_FIELD(reason), JSON_FIELD(hash_summary), JSON_FIELD(events)};
}

#undef JSON_FIELD
#undef READ_FIELD

}  // namespace cloud::fleet_operations
