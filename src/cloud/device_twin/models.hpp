#pragma once

#include <map>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace cloud::device_twin {

struct FirmwareState {
  std::string current_version{"1.0.0"};
  std::string target_version{"1.0.0"};
  std::string hardware_revision{"revA"};
  std::string update_state{"idle"};
};

struct DrawerHealth {
  int score{100};
  int jam_count{0};
  int dispense_failures{0};
  double scale_drift_g{0.0};
  int self_test_failures{0};
  int communication_outages{0};
  int hopper_depletion_events{0};
  int motor_cycles{0};
  double transaction_latency_ms{0.0};
  double uptime_percent{100.0};
  std::vector<std::string> reasons;
};

struct DenominationInventory {
  std::string denomination;
  std::string currency_code{"USD"};
  int quantity{0};
  int capacity{0};
  double consumption_per_hour{0.0};
  std::string last_refill_at;
};

struct InventoryState {
  std::string currency_code{"USD"};
  std::string country_code;
  std::string region;
  std::vector<std::string> compliance_tags;
  std::map<std::string, DenominationInventory> denominations;
};

struct FaultEvent {
  std::string id;
  std::string type;
  std::string severity{"warning"};
  std::string message;
  std::string occurred_at;
  bool resolved{false};
};

struct MaintenanceRecord {
  std::string id;
  std::string type;
  std::string notes;
  std::string performed_by;
  std::string performed_at;
};

struct HistoryEvent {
  std::string id;
  std::string type;
  std::string summary;
  std::string occurred_at;
  nlohmann::json metadata = nlohmann::json::object();
};

struct MaintenanceAssessment {
  double predicted_failure_probability{0.0};
  std::string recommended_service_date;
  std::string maintenance_priority{"low"};
  std::vector<std::string> drivers;
};

struct InventoryForecast {
  double hours_until_empty{0.0};
  std::vector<std::string> refill_recommendations;
  std::vector<std::string> depletion_trends;
};

struct Alert {
  std::string id;
  std::string type;
  std::string severity{"warning"};
  std::string message;
  std::string created_at;
};

struct DrawerTwin {
  std::string drawer_id;
  std::string device_id;
  std::string merchant_id;
  std::string region;
  std::string environment{"development"};
  std::string deployment_channel{"dev"};
  std::string country_code;
  std::vector<std::string> compliance_tags;
  bool enrolled{false};
  bool disabled{false};
  std::string enrollment_state{"local"};
  std::string enrollment_token_hash;
  std::string sync_status{"local"};
  std::string last_synced_at;
  int revision{0};
  int remote_revision{0};
  bool conflict{false};
  std::string conflict_reason;
  FirmwareState firmware;
  DrawerHealth health;
  InventoryState inventory;
  std::vector<FaultEvent> fault_history;
  std::vector<MaintenanceRecord> maintenance_history;
  std::string connectivity_status{"offline"};
  std::string first_seen_at;
  std::string last_telemetry_at;
  std::vector<HistoryEvent> history;
  MaintenanceAssessment maintenance;
  InventoryForecast inventory_forecast;
  std::vector<Alert> alerts;
};

bool valid_currency_code(const std::string& code);

void to_json(nlohmann::json& j, const FirmwareState& v);
void to_json(nlohmann::json& j, const DrawerHealth& v);
void to_json(nlohmann::json& j, const DenominationInventory& v);
void to_json(nlohmann::json& j, const InventoryState& v);
void to_json(nlohmann::json& j, const FaultEvent& v);
void to_json(nlohmann::json& j, const MaintenanceRecord& v);
void to_json(nlohmann::json& j, const HistoryEvent& v);
void to_json(nlohmann::json& j, const MaintenanceAssessment& v);
void to_json(nlohmann::json& j, const InventoryForecast& v);
void to_json(nlohmann::json& j, const Alert& v);
void to_json(nlohmann::json& j, const DrawerTwin& v);
void from_json(const nlohmann::json& j, FirmwareState& v);
void from_json(const nlohmann::json& j, DrawerHealth& v);
void from_json(const nlohmann::json& j, DenominationInventory& v);
void from_json(const nlohmann::json& j, InventoryState& v);
void from_json(const nlohmann::json& j, FaultEvent& v);
void from_json(const nlohmann::json& j, MaintenanceRecord& v);
void from_json(const nlohmann::json& j, HistoryEvent& v);
void from_json(const nlohmann::json& j, MaintenanceAssessment& v);
void from_json(const nlohmann::json& j, InventoryForecast& v);
void from_json(const nlohmann::json& j, Alert& v);
void from_json(const nlohmann::json& j, DrawerTwin& v);

}  // namespace cloud::device_twin
