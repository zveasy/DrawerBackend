#include "cloud/device_twin/models.hpp"

namespace cloud::device_twin {

void to_json(nlohmann::json& j, const FirmwareState& v) {
  j = {{"current_version", v.current_version},
       {"target_version", v.target_version},
       {"hardware_revision", v.hardware_revision},
       {"update_state", v.update_state},
       {"firmware_mismatch", v.current_version != v.target_version}};
}

void to_json(nlohmann::json& j, const DrawerHealth& v) {
  j = {{"score", v.score},
       {"jam_count", v.jam_count},
       {"dispense_failures", v.dispense_failures},
       {"scale_drift_g", v.scale_drift_g},
       {"self_test_failures", v.self_test_failures},
       {"communication_outages", v.communication_outages},
       {"hopper_depletion_events", v.hopper_depletion_events},
       {"motor_cycles", v.motor_cycles},
       {"transaction_latency_ms", v.transaction_latency_ms},
       {"uptime_percent", v.uptime_percent},
       {"reasons", v.reasons}};
}

void to_json(nlohmann::json& j, const DenominationInventory& v) {
  j = {{"denomination", v.denomination},
       {"quantity", v.quantity},
       {"capacity", v.capacity},
       {"consumption_per_hour", v.consumption_per_hour},
       {"last_refill_at", v.last_refill_at}};
}

void to_json(nlohmann::json& j, const InventoryState& v) {
  j = nlohmann::json::object();
  for (const auto& item : v.denominations) {
    j[item.first] = item.second;
  }
}

void to_json(nlohmann::json& j, const FaultEvent& v) {
  j = {{"id", v.id},
       {"type", v.type},
       {"severity", v.severity},
       {"message", v.message},
       {"occurred_at", v.occurred_at},
       {"resolved", v.resolved}};
}

void to_json(nlohmann::json& j, const MaintenanceRecord& v) {
  j = {{"id", v.id},
       {"type", v.type},
       {"notes", v.notes},
       {"performed_by", v.performed_by},
       {"performed_at", v.performed_at}};
}

void to_json(nlohmann::json& j, const HistoryEvent& v) {
  j = {{"id", v.id},
       {"type", v.type},
       {"summary", v.summary},
       {"occurred_at", v.occurred_at},
       {"metadata", v.metadata}};
}

void to_json(nlohmann::json& j, const MaintenanceAssessment& v) {
  j = {{"predicted_failure_probability", v.predicted_failure_probability},
       {"recommended_service_date", v.recommended_service_date},
       {"maintenance_priority", v.maintenance_priority},
       {"drivers", v.drivers}};
}

void to_json(nlohmann::json& j, const InventoryForecast& v) {
  j = {{"hours_until_empty", v.hours_until_empty},
       {"refill_recommendations", v.refill_recommendations},
       {"depletion_trends", v.depletion_trends}};
}

void to_json(nlohmann::json& j, const Alert& v) {
  j = {{"id", v.id},
       {"type", v.type},
       {"severity", v.severity},
       {"message", v.message},
       {"created_at", v.created_at}};
}

void to_json(nlohmann::json& j, const DrawerTwin& v) {
  j = {{"drawer_id", v.drawer_id},
       {"merchant_id", v.merchant_id},
       {"firmware", v.firmware},
       {"hardware_revision", v.firmware.hardware_revision},
       {"health", v.health},
       {"inventory", v.inventory},
       {"fault_history", v.fault_history},
       {"maintenance_history", v.maintenance_history},
       {"connectivity_status", v.connectivity_status},
       {"first_seen_at", v.first_seen_at},
       {"last_telemetry_at", v.last_telemetry_at},
       {"history", v.history},
       {"maintenance", v.maintenance},
       {"inventory_forecast", v.inventory_forecast},
       {"alerts", v.alerts}};
}

void from_json(const nlohmann::json& j, FirmwareState& v) {
  v.current_version = j.value("current_version", v.current_version);
  v.target_version = j.value("target_version", v.target_version);
  v.hardware_revision = j.value("hardware_revision", v.hardware_revision);
  v.update_state = j.value("update_state", v.update_state);
}

void from_json(const nlohmann::json& j, DrawerHealth& v) {
  v.score = j.value("score", v.score);
  v.jam_count = j.value("jam_count", v.jam_count);
  v.dispense_failures = j.value("dispense_failures", v.dispense_failures);
  v.scale_drift_g = j.value("scale_drift_g", v.scale_drift_g);
  v.self_test_failures = j.value("self_test_failures", v.self_test_failures);
  v.communication_outages = j.value("communication_outages", v.communication_outages);
  v.hopper_depletion_events = j.value("hopper_depletion_events", v.hopper_depletion_events);
  v.motor_cycles = j.value("motor_cycles", v.motor_cycles);
  v.transaction_latency_ms = j.value("transaction_latency_ms", v.transaction_latency_ms);
  v.uptime_percent = j.value("uptime_percent", v.uptime_percent);
  v.reasons = j.value("reasons", v.reasons);
}

void from_json(const nlohmann::json& j, DenominationInventory& v) {
  v.denomination = j.value("denomination", v.denomination);
  v.quantity = j.value("quantity", v.quantity);
  v.capacity = j.value("capacity", v.capacity);
  v.consumption_per_hour = j.value("consumption_per_hour", v.consumption_per_hour);
  v.last_refill_at = j.value("last_refill_at", v.last_refill_at);
}

void from_json(const nlohmann::json& j, InventoryState& v) {
  v.denominations.clear();
  if (!j.is_object()) return;
  for (const auto& item : j.items()) {
    v.denominations[item.key()] = item.value().get<DenominationInventory>();
  }
}

void from_json(const nlohmann::json& j, FaultEvent& v) {
  v.id = j.value("id", v.id);
  v.type = j.value("type", v.type);
  v.severity = j.value("severity", v.severity);
  v.message = j.value("message", v.message);
  v.occurred_at = j.value("occurred_at", v.occurred_at);
  v.resolved = j.value("resolved", v.resolved);
}

void from_json(const nlohmann::json& j, MaintenanceRecord& v) {
  v.id = j.value("id", v.id);
  v.type = j.value("type", v.type);
  v.notes = j.value("notes", v.notes);
  v.performed_by = j.value("performed_by", v.performed_by);
  v.performed_at = j.value("performed_at", v.performed_at);
}

void from_json(const nlohmann::json& j, HistoryEvent& v) {
  v.id = j.value("id", v.id);
  v.type = j.value("type", v.type);
  v.summary = j.value("summary", v.summary);
  v.occurred_at = j.value("occurred_at", v.occurred_at);
  v.metadata = j.value("metadata", nlohmann::json::object());
}

void from_json(const nlohmann::json& j, MaintenanceAssessment& v) {
  v.predicted_failure_probability =
      j.value("predicted_failure_probability", v.predicted_failure_probability);
  v.recommended_service_date = j.value("recommended_service_date", v.recommended_service_date);
  v.maintenance_priority = j.value("maintenance_priority", v.maintenance_priority);
  v.drivers = j.value("drivers", v.drivers);
}

void from_json(const nlohmann::json& j, InventoryForecast& v) {
  v.hours_until_empty = j.value("hours_until_empty", v.hours_until_empty);
  v.refill_recommendations = j.value("refill_recommendations", v.refill_recommendations);
  v.depletion_trends = j.value("depletion_trends", v.depletion_trends);
}

void from_json(const nlohmann::json& j, Alert& v) {
  v.id = j.value("id", v.id);
  v.type = j.value("type", v.type);
  v.severity = j.value("severity", v.severity);
  v.message = j.value("message", v.message);
  v.created_at = j.value("created_at", v.created_at);
}

void from_json(const nlohmann::json& j, DrawerTwin& v) {
  v.drawer_id = j.value("drawer_id", v.drawer_id);
  v.merchant_id = j.value("merchant_id", v.merchant_id);
  if (j.contains("firmware")) v.firmware = j.at("firmware").get<FirmwareState>();
  if (j.contains("health")) v.health = j.at("health").get<DrawerHealth>();
  if (j.contains("inventory")) v.inventory = j.at("inventory").get<InventoryState>();
  v.fault_history = j.value("fault_history", v.fault_history);
  v.maintenance_history = j.value("maintenance_history", v.maintenance_history);
  v.connectivity_status = j.value("connectivity_status", v.connectivity_status);
  v.first_seen_at = j.value("first_seen_at", v.first_seen_at);
  v.last_telemetry_at = j.value("last_telemetry_at", v.last_telemetry_at);
  v.history = j.value("history", v.history);
  if (j.contains("maintenance")) v.maintenance = j.at("maintenance").get<MaintenanceAssessment>();
  if (j.contains("inventory_forecast")) {
    v.inventory_forecast = j.at("inventory_forecast").get<InventoryForecast>();
  }
  v.alerts = j.value("alerts", v.alerts);
}

}  // namespace cloud::device_twin
