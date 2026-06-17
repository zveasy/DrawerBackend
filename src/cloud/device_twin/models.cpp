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

}  // namespace cloud::device_twin
