#include "edge_platform/models.hpp"

namespace edge_platform {

void to_json(nlohmann::json& j, const EdgeDevice& v) {
  j = {{"device_id", v.device_id}, {"tenant_id", v.tenant_id}, {"merchant_id", v.merchant_id},
       {"branch_id", v.branch_id}, {"region", v.region}, {"device_type", v.device_type},
       {"driver_id", v.driver_id}, {"firmware_version", v.firmware_version},
       {"capabilities", v.capabilities}, {"labels", v.labels}};
}
void from_json(const nlohmann::json& j, EdgeDevice& v) {
  v.device_id = j.value("device_id", v.device_id);
  v.tenant_id = j.value("tenant_id", v.tenant_id);
  v.merchant_id = j.value("merchant_id", v.merchant_id);
  v.branch_id = j.value("branch_id", v.branch_id);
  v.region = j.value("region", v.region);
  v.device_type = j.value("device_type", v.device_type);
  v.driver_id = j.value("driver_id", v.driver_id);
  v.firmware_version = j.value("firmware_version", v.firmware_version);
  v.capabilities = j.value("capabilities", v.capabilities);
  v.labels = j.value("labels", v.labels);
}

void to_json(nlohmann::json& j, const CapabilityMetadata& v) {
  j = {{"capability", v.capability}, {"description", v.description}, {"high_risk", v.high_risk}};
}
void from_json(const nlohmann::json& j, CapabilityMetadata& v) {
  v.capability = j.value("capability", v.capability);
  v.description = j.value("description", v.description);
  v.high_risk = j.value("high_risk", v.high_risk);
}

void to_json(nlohmann::json& j, const DriverMetadata& v) {
  j = {{"driver_id", v.driver_id}, {"vendor", v.vendor}, {"device_type", v.device_type},
       {"version", v.version}, {"health", v.health}, {"supported_capabilities", v.supported_capabilities}};
}
void from_json(const nlohmann::json& j, DriverMetadata& v) {
  v.driver_id = j.value("driver_id", v.driver_id);
  v.vendor = j.value("vendor", v.vendor);
  v.device_type = j.value("device_type", v.device_type);
  v.version = j.value("version", v.version);
  v.health = j.value("health", v.health);
  v.supported_capabilities = j.value("supported_capabilities", v.supported_capabilities);
}

void to_json(nlohmann::json& j, const EdgeCommandRequest& v) {
  j = {{"command_id", v.command_id}, {"device_id", v.device_id}, {"command", v.command},
       {"parameters", v.parameters}, {"actor_id", v.actor_id}, {"requested_at", v.requested_at}};
}
void from_json(const nlohmann::json& j, EdgeCommandRequest& v) {
  v.command_id = j.value("command_id", v.command_id);
  v.device_id = j.value("device_id", v.device_id);
  v.command = j.value("command", v.command);
  v.parameters = j.value("parameters", nlohmann::json::object());
  v.actor_id = j.value("actor_id", v.actor_id);
  v.requested_at = j.value("requested_at", v.requested_at);
}

void to_json(nlohmann::json& j, const EdgeCommandResult& v) {
  j = {{"command_id", v.command_id}, {"device_id", v.device_id}, {"command", v.command},
       {"accepted", v.accepted}, {"executed", v.executed}, {"status", v.status},
       {"reason", v.reason}, {"completed_at", v.completed_at}};
}
void from_json(const nlohmann::json& j, EdgeCommandResult& v) {
  v.command_id = j.value("command_id", v.command_id);
  v.device_id = j.value("device_id", v.device_id);
  v.command = j.value("command", v.command);
  v.accepted = j.value("accepted", v.accepted);
  v.executed = j.value("executed", v.executed);
  v.status = j.value("status", v.status);
  v.reason = j.value("reason", v.reason);
  v.completed_at = j.value("completed_at", v.completed_at);
}

void to_json(nlohmann::json& j, const EdgeInventoryCompartment& v) {
  j = {{"compartment_id", v.compartment_id}, {"compartment_type", v.compartment_type},
       {"currency", v.currency}, {"denominations", v.denominations}, {"status", v.status}};
}
void from_json(const nlohmann::json& j, EdgeInventoryCompartment& v) {
  v.compartment_id = j.value("compartment_id", v.compartment_id);
  v.compartment_type = j.value("compartment_type", v.compartment_type);
  v.currency = j.value("currency", v.currency);
  v.denominations = j.value("denominations", v.denominations);
  v.status = j.value("status", v.status);
}

void to_json(nlohmann::json& j, const EdgeInventoryView& v) {
  j = {{"device_id", v.device_id}, {"compartments", v.compartments}, {"updated_at", v.updated_at}};
}
void from_json(const nlohmann::json& j, EdgeInventoryView& v) {
  v.device_id = j.value("device_id", v.device_id);
  v.compartments = j.value("compartments", v.compartments);
  v.updated_at = j.value("updated_at", v.updated_at);
}

void to_json(nlohmann::json& j, const EdgeHealthSummary& v) {
  j = {{"device_id", v.device_id}, {"device_type", v.device_type}, {"score", v.score},
       {"factors", v.factors}, {"explanations", v.explanations}};
}
void from_json(const nlohmann::json& j, EdgeHealthSummary& v) {
  v.device_id = j.value("device_id", v.device_id);
  v.device_type = j.value("device_type", v.device_type);
  v.score = j.value("score", v.score);
  v.factors = j.value("factors", v.factors);
  v.explanations = j.value("explanations", v.explanations);
}

void to_json(nlohmann::json& j, const SimulatorState& v) {
  j = {{"device_id", v.device_id}, {"online", v.online}, {"faults", v.faults},
       {"sensor_telemetry", v.sensor_telemetry}};
}
void from_json(const nlohmann::json& j, SimulatorState& v) {
  v.device_id = j.value("device_id", v.device_id);
  v.online = j.value("online", v.online);
  v.faults = j.value("faults", v.faults);
  v.sensor_telemetry = j.value("sensor_telemetry", nlohmann::json::object());
}

void to_json(nlohmann::json& j, const EdgeEvent& v) {
  j = {{"event_id", v.event_id}, {"tenant_id", v.tenant_id}, {"device_id", v.device_id},
       {"event_type", v.event_type}, {"occurred_at", v.occurred_at}, {"metadata", v.metadata}};
}
void from_json(const nlohmann::json& j, EdgeEvent& v) {
  v.event_id = j.value("event_id", v.event_id);
  v.tenant_id = j.value("tenant_id", v.tenant_id);
  v.device_id = j.value("device_id", v.device_id);
  v.event_type = j.value("event_type", v.event_type);
  v.occurred_at = j.value("occurred_at", v.occurred_at);
  v.metadata = j.value("metadata", nlohmann::json::object());
}

}  // namespace edge_platform
