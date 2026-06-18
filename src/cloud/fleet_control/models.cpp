#include "cloud/fleet_control/models.hpp"

namespace cloud::fleet_control {

void to_json(nlohmann::json& j, const Organization& v) {
  j = {{"organization_id", v.organization_id}, {"name", v.name}, {"tags", v.tags}, {"labels", v.labels}};
}
void from_json(const nlohmann::json& j, Organization& v) {
  v.organization_id = j.value("organization_id", v.organization_id);
  v.name = j.value("name", v.name);
  v.tags = j.value("tags", v.tags);
  v.labels = j.value("labels", v.labels);
}

void to_json(nlohmann::json& j, const Region& v) {
  j = {{"region_id", v.region_id}, {"organization_id", v.organization_id}, {"name", v.name},
       {"tags", v.tags}, {"labels", v.labels}};
}
void from_json(const nlohmann::json& j, Region& v) {
  v.region_id = j.value("region_id", v.region_id);
  v.organization_id = j.value("organization_id", v.organization_id);
  v.name = j.value("name", v.name);
  v.tags = j.value("tags", v.tags);
  v.labels = j.value("labels", v.labels);
}

void to_json(nlohmann::json& j, const Merchant& v) {
  j = {{"merchant_id", v.merchant_id}, {"organization_id", v.organization_id}, {"name", v.name},
       {"tags", v.tags}, {"labels", v.labels}};
}
void from_json(const nlohmann::json& j, Merchant& v) {
  v.merchant_id = j.value("merchant_id", v.merchant_id);
  v.organization_id = j.value("organization_id", v.organization_id);
  v.name = j.value("name", v.name);
  v.tags = j.value("tags", v.tags);
  v.labels = j.value("labels", v.labels);
}

void to_json(nlohmann::json& j, const Branch& v) {
  j = {{"branch_id", v.branch_id}, {"merchant_id", v.merchant_id},
       {"organization_id", v.organization_id}, {"region_id", v.region_id}, {"name", v.name},
       {"tags", v.tags}, {"labels", v.labels}};
}
void from_json(const nlohmann::json& j, Branch& v) {
  v.branch_id = j.value("branch_id", v.branch_id);
  v.merchant_id = j.value("merchant_id", v.merchant_id);
  v.organization_id = j.value("organization_id", v.organization_id);
  v.region_id = j.value("region_id", v.region_id);
  v.name = j.value("name", v.name);
  v.tags = j.value("tags", v.tags);
  v.labels = j.value("labels", v.labels);
}

void to_json(nlohmann::json& j, const DeviceRegistryRecord& v) {
  j = {{"device_id", v.device_id},
       {"device_type", v.device_type},
       {"organization_id", v.organization_id},
       {"merchant_id", v.merchant_id},
       {"branch_id", v.branch_id},
       {"region", v.region},
       {"environment", v.environment},
       {"deployment_channel", v.deployment_channel},
       {"firmware_version", v.firmware_version},
       {"enrollment_status", v.enrollment_status},
       {"certificate_identity_status", v.certificate_identity_status},
       {"last_heartbeat_at", v.last_heartbeat_at},
       {"last_seen_at", v.last_seen_at},
       {"connectivity_status", v.connectivity_status},
       {"health_status", v.health_status},
       {"health_score", v.health_score},
       {"transaction_failures", v.transaction_failures},
       {"ota_failures", v.ota_failures},
       {"sync_failures", v.sync_failures},
       {"capabilities", v.capabilities},
       {"tags", v.tags},
       {"labels", v.labels}};
}
void from_json(const nlohmann::json& j, DeviceRegistryRecord& v) {
  v.device_id = j.value("device_id", v.device_id);
  v.device_type = j.value("device_type", v.device_type);
  v.organization_id = j.value("organization_id", v.organization_id);
  v.merchant_id = j.value("merchant_id", v.merchant_id);
  v.branch_id = j.value("branch_id", v.branch_id);
  v.region = j.value("region", v.region);
  v.environment = j.value("environment", v.environment);
  v.deployment_channel = j.value("deployment_channel", v.deployment_channel);
  v.firmware_version = j.value("firmware_version", v.firmware_version);
  v.enrollment_status = j.value("enrollment_status", v.enrollment_status);
  v.certificate_identity_status = j.value("certificate_identity_status", v.certificate_identity_status);
  v.last_heartbeat_at = j.value("last_heartbeat_at", v.last_heartbeat_at);
  v.last_seen_at = j.value("last_seen_at", v.last_seen_at);
  v.connectivity_status = j.value("connectivity_status", v.connectivity_status);
  v.health_status = j.value("health_status", v.health_status);
  v.health_score = j.value("health_score", v.health_score);
  v.transaction_failures = j.value("transaction_failures", v.transaction_failures);
  v.ota_failures = j.value("ota_failures", v.ota_failures);
  v.sync_failures = j.value("sync_failures", v.sync_failures);
  v.capabilities = j.value("capabilities", v.capabilities);
  v.tags = j.value("tags", v.tags);
  v.labels = j.value("labels", v.labels);
}

void to_json(nlohmann::json& j, const HeartbeatMessage& v) {
  j = {{"device_id", v.device_id}, {"sent_at_epoch", v.sent_at_epoch},
       {"received_at_epoch", v.received_at_epoch}, {"latency_ms", v.latency_ms}};
}
void from_json(const nlohmann::json& j, HeartbeatMessage& v) {
  v.device_id = j.value("device_id", v.device_id);
  v.sent_at_epoch = j.value("sent_at_epoch", v.sent_at_epoch);
  v.received_at_epoch = j.value("received_at_epoch", v.received_at_epoch);
  v.latency_ms = j.value("latency_ms", v.latency_ms);
}

void to_json(nlohmann::json& j, const RemoteCommand& v) {
  j = {{"command_id", v.command_id}, {"organization_id", v.organization_id},
       {"device_id", v.device_id}, {"type", v.type}, {"status", v.status},
       {"created_at", v.created_at}, {"delivered_at", v.delivered_at},
       {"acknowledged_at", v.acknowledged_at}, {"failed_at", v.failed_at},
       {"expires_at_epoch", v.expires_at_epoch}, {"retry_count", v.retry_count},
       {"max_retries", v.max_retries}, {"failure_reason", v.failure_reason}};
}
void from_json(const nlohmann::json& j, RemoteCommand& v) {
  v.command_id = j.value("command_id", v.command_id);
  v.organization_id = j.value("organization_id", v.organization_id);
  v.device_id = j.value("device_id", v.device_id);
  v.type = j.value("type", v.type);
  v.status = j.value("status", v.status);
  v.created_at = j.value("created_at", v.created_at);
  v.delivered_at = j.value("delivered_at", v.delivered_at);
  v.acknowledged_at = j.value("acknowledged_at", v.acknowledged_at);
  v.failed_at = j.value("failed_at", v.failed_at);
  v.expires_at_epoch = j.value("expires_at_epoch", v.expires_at_epoch);
  v.retry_count = j.value("retry_count", v.retry_count);
  v.max_retries = j.value("max_retries", v.max_retries);
  v.failure_reason = j.value("failure_reason", v.failure_reason);
}

void to_json(nlohmann::json& j, const FleetAlert& v) {
  j = {{"alert_id", v.alert_id}, {"organization_id", v.organization_id},
       {"device_id", v.device_id}, {"type", v.type}, {"severity", v.severity},
       {"message", v.message}, {"created_at", v.created_at},
       {"acknowledged", v.acknowledged}, {"acknowledged_at", v.acknowledged_at}};
}
void from_json(const nlohmann::json& j, FleetAlert& v) {
  v.alert_id = j.value("alert_id", v.alert_id);
  v.organization_id = j.value("organization_id", v.organization_id);
  v.device_id = j.value("device_id", v.device_id);
  v.type = j.value("type", v.type);
  v.severity = j.value("severity", v.severity);
  v.message = j.value("message", v.message);
  v.created_at = j.value("created_at", v.created_at);
  v.acknowledged = j.value("acknowledged", v.acknowledged);
  v.acknowledged_at = j.value("acknowledged_at", v.acknowledged_at);
}

void to_json(nlohmann::json& j, const FleetEvent& v) {
  j = {{"event_id", v.event_id}, {"organization_id", v.organization_id},
       {"device_id", v.device_id}, {"type", v.type}, {"occurred_at", v.occurred_at},
       {"metadata", v.metadata}};
}
void from_json(const nlohmann::json& j, FleetEvent& v) {
  v.event_id = j.value("event_id", v.event_id);
  v.organization_id = j.value("organization_id", v.organization_id);
  v.device_id = j.value("device_id", v.device_id);
  v.type = j.value("type", v.type);
  v.occurred_at = j.value("occurred_at", v.occurred_at);
  v.metadata = j.value("metadata", nlohmann::json::object());
}

void to_json(nlohmann::json& j, const FleetHealthSummary& v) {
  j = {{"device_health_score", v.device_health_score},
       {"communication_health_score", v.communication_health_score},
       {"sync_health_score", v.sync_health_score},
       {"ota_health_score", v.ota_health_score},
       {"certificate_health_score", v.certificate_health_score},
       {"aggregate_score", v.aggregate_score}};
}

void to_json(nlohmann::json& j, const FleetMetrics& v) {
  j = {{"online_devices", v.online_devices}, {"offline_devices", v.offline_devices},
       {"command_success_rate", v.command_success_rate},
       {"command_failure_rate", v.command_failure_rate},
       {"average_heartbeat_latency_ms", v.average_heartbeat_latency_ms},
       {"fleet_availability", v.fleet_availability}, {"alert_counts", v.alert_counts},
       {"sync_success_count", v.sync_success_count}, {"sync_failure_count", v.sync_failure_count}};
}

void to_json(nlohmann::json& j, const DashboardView& v) {
  j = {{"scope_type", v.scope_type}, {"scope_id", v.scope_id}, {"device_count", v.device_count},
       {"online_devices", v.online_devices}, {"offline_devices", v.offline_devices},
       {"firmware_distribution", v.firmware_distribution}, {"health", v.health},
       {"alert_summary", v.alert_summary}};
}

}  // namespace cloud::fleet_control
