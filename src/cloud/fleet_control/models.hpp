#pragma once

#include <map>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

namespace cloud::fleet_control {

using Labels = std::map<std::string, std::string>;

struct Organization {
  std::string organization_id;
  std::string name;
  std::vector<std::string> tags;
  Labels labels;
};

struct Region {
  std::string region_id;
  std::string organization_id;
  std::string name;
  std::vector<std::string> tags;
  Labels labels;
};

struct Merchant {
  std::string merchant_id;
  std::string organization_id;
  std::string name;
  std::vector<std::string> tags;
  Labels labels;
};

struct Branch {
  std::string branch_id;
  std::string merchant_id;
  std::string organization_id;
  std::string region_id;
  std::string name;
  std::vector<std::string> tags;
  Labels labels;
};

struct DeviceRegistryRecord {
  std::string device_id;
  std::string device_type{"cash_drawer"};
  std::string organization_id;
  std::string merchant_id;
  std::string branch_id;
  std::string region;
  std::string environment;
  std::string deployment_channel;
  std::string firmware_version;
  std::string enrollment_status{"unenrolled"};
  std::string certificate_identity_status{"unknown"};
  std::string last_heartbeat_at;
  std::string last_seen_at;
  std::string connectivity_status{"offline"};
  std::string health_status{"unknown"};
  int health_score{100};
  int transaction_failures{0};
  int ota_failures{0};
  int sync_failures{0};
  std::vector<std::string> capabilities;
  std::vector<std::string> tags;
  Labels labels;
};

struct HeartbeatMessage {
  std::string device_id;
  long sent_at_epoch{0};
  long received_at_epoch{0};
  double latency_ms{0.0};
};

struct RemoteCommand {
  std::string command_id;
  std::string organization_id;
  std::string device_id;
  std::string type;
  std::string status{"pending"};
  std::string created_at;
  std::string delivered_at;
  std::string acknowledged_at;
  std::string failed_at;
  long expires_at_epoch{0};
  int retry_count{0};
  int max_retries{3};
  std::string failure_reason;
};

struct FleetAlert {
  std::string alert_id;
  std::string organization_id;
  std::string device_id;
  std::string type;
  std::string severity{"warning"};
  std::string message;
  std::string created_at;
  bool acknowledged{false};
  std::string acknowledged_at;
};

struct FleetEvent {
  std::string event_id;
  std::string organization_id;
  std::string device_id;
  std::string type;
  std::string occurred_at;
  nlohmann::json metadata = nlohmann::json::object();
};

struct FleetHealthSummary {
  int device_health_score{100};
  int communication_health_score{100};
  int sync_health_score{100};
  int ota_health_score{100};
  int certificate_health_score{100};
  int aggregate_score{100};
};

struct FleetMetrics {
  int online_devices{0};
  int offline_devices{0};
  double command_success_rate{0.0};
  double command_failure_rate{0.0};
  double average_heartbeat_latency_ms{0.0};
  double fleet_availability{0.0};
  std::map<std::string, int> alert_counts;
  int sync_success_count{0};
  int sync_failure_count{0};
};

struct DashboardView {
  std::string scope_type;
  std::string scope_id;
  int device_count{0};
  int online_devices{0};
  int offline_devices{0};
  std::map<std::string, int> firmware_distribution;
  FleetHealthSummary health;
  std::map<std::string, int> alert_summary;
};

struct DeviceFilter {
  std::string organization_id;
  std::string merchant_id;
  std::string branch_id;
  std::string region;
  std::string environment;
  std::string deployment_channel;
  std::string health_status;
  std::string connectivity_status;
  std::string tag;
  std::string label_key;
  std::string label_value;
  std::string text;
};

void to_json(nlohmann::json& j, const Organization& v);
void from_json(const nlohmann::json& j, Organization& v);
void to_json(nlohmann::json& j, const Region& v);
void from_json(const nlohmann::json& j, Region& v);
void to_json(nlohmann::json& j, const Merchant& v);
void from_json(const nlohmann::json& j, Merchant& v);
void to_json(nlohmann::json& j, const Branch& v);
void from_json(const nlohmann::json& j, Branch& v);
void to_json(nlohmann::json& j, const DeviceRegistryRecord& v);
void from_json(const nlohmann::json& j, DeviceRegistryRecord& v);
void to_json(nlohmann::json& j, const HeartbeatMessage& v);
void from_json(const nlohmann::json& j, HeartbeatMessage& v);
void to_json(nlohmann::json& j, const RemoteCommand& v);
void from_json(const nlohmann::json& j, RemoteCommand& v);
void to_json(nlohmann::json& j, const FleetAlert& v);
void from_json(const nlohmann::json& j, FleetAlert& v);
void to_json(nlohmann::json& j, const FleetEvent& v);
void from_json(const nlohmann::json& j, FleetEvent& v);
void to_json(nlohmann::json& j, const FleetHealthSummary& v);
void to_json(nlohmann::json& j, const FleetMetrics& v);
void to_json(nlohmann::json& j, const DashboardView& v);

}  // namespace cloud::fleet_control
