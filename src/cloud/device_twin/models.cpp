#include "cloud/device_twin/models.hpp"

namespace cloud::device_twin {

bool valid_currency_code(const std::string& code) {
  if (code.size() != 3) return false;
  for (char c : code) {
    if (c < 'A' || c > 'Z') return false;
  }
  return true;
}

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
       {"currency_code", v.currency_code},
       {"quantity", v.quantity},
       {"capacity", v.capacity},
       {"consumption_per_hour", v.consumption_per_hour},
       {"last_refill_at", v.last_refill_at}};
}

void to_json(nlohmann::json& j, const InventoryState& v) {
  j = {{"currency_code", v.currency_code},
       {"country_code", v.country_code},
       {"region", v.region},
       {"compliance_tags", v.compliance_tags},
       {"denominations", nlohmann::json::object()}};
  for (const auto& item : v.denominations) {
    j["denominations"][item.first] = item.second;
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
       {"device_id", v.device_id},
       {"merchant_id", v.merchant_id},
       {"region", v.region},
       {"environment", v.environment},
       {"deployment_channel", v.deployment_channel},
       {"country_code", v.country_code},
       {"compliance_tags", v.compliance_tags},
       {"enrolled", v.enrolled},
       {"disabled", v.disabled},
       {"enrollment_state", v.enrollment_state},
       {"enrollment_token_hash", v.enrollment_token_hash},
       {"sync_status", v.sync_status},
       {"last_synced_at", v.last_synced_at},
       {"revision", v.revision},
       {"remote_revision", v.remote_revision},
       {"conflict", v.conflict},
       {"conflict_reason", v.conflict_reason},
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
  v.currency_code = j.value("currency_code", v.currency_code);
  v.quantity = j.value("quantity", v.quantity);
  v.capacity = j.value("capacity", v.capacity);
  v.consumption_per_hour = j.value("consumption_per_hour", v.consumption_per_hour);
  v.last_refill_at = j.value("last_refill_at", v.last_refill_at);
}

void from_json(const nlohmann::json& j, InventoryState& v) {
  v.denominations.clear();
  if (!j.is_object()) return;
  v.currency_code = j.value("currency_code", v.currency_code);
  v.country_code = j.value("country_code", v.country_code);
  v.region = j.value("region", v.region);
  v.compliance_tags = j.value("compliance_tags", v.compliance_tags);
  const auto& denoms = j.contains("denominations") ? j.at("denominations") : j;
  for (const auto& item : denoms.items()) {
    if (item.key() == "currency_code" || item.key() == "country_code" || item.key() == "region" ||
        item.key() == "compliance_tags") {
      continue;
    }
    auto denom = item.value().get<DenominationInventory>();
    if (denom.currency_code.empty()) denom.currency_code = v.currency_code;
    v.denominations[item.key()] = denom;
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
  v.device_id = j.value("device_id", v.device_id);
  v.merchant_id = j.value("merchant_id", v.merchant_id);
  v.region = j.value("region", v.region);
  v.environment = j.value("environment", v.environment);
  v.deployment_channel = j.value("deployment_channel", v.deployment_channel);
  v.country_code = j.value("country_code", v.country_code);
  v.compliance_tags = j.value("compliance_tags", v.compliance_tags);
  v.enrolled = j.value("enrolled", v.enrolled);
  v.disabled = j.value("disabled", v.disabled);
  v.enrollment_state = j.value("enrollment_state", v.enrollment_state);
  v.enrollment_token_hash = j.value("enrollment_token_hash", v.enrollment_token_hash);
  v.sync_status = j.value("sync_status", v.sync_status);
  v.last_synced_at = j.value("last_synced_at", v.last_synced_at);
  v.revision = j.value("revision", v.revision);
  v.remote_revision = j.value("remote_revision", v.remote_revision);
  v.conflict = j.value("conflict", v.conflict);
  v.conflict_reason = j.value("conflict_reason", v.conflict_reason);
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
