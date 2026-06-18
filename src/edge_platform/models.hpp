#pragma once

#include <map>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

namespace edge_platform {

using Labels = std::map<std::string, std::string>;
using DenominationBreakdown = std::map<std::string, int>;

struct EdgeDevice {
  std::string device_id;
  std::string tenant_id;
  std::string merchant_id;
  std::string branch_id;
  std::string region;
  std::string device_type{"cash_drawer"};
  std::string driver_id;
  std::string firmware_version;
  std::vector<std::string> capabilities;
  Labels labels;
};

struct CapabilityMetadata {
  std::string capability;
  std::string description;
  bool high_risk{false};
};

struct DriverMetadata {
  std::string driver_id;
  std::string vendor;
  std::string device_type;
  std::string version;
  std::string health{"healthy"};
  std::vector<std::string> supported_capabilities;
};

struct EdgeCommandRequest {
  std::string command_id;
  std::string device_id;
  std::string command;
  nlohmann::json parameters = nlohmann::json::object();
  std::string actor_id;
  long requested_at{0};
};

struct EdgeCommandResult {
  std::string command_id;
  std::string device_id;
  std::string command;
  bool accepted{false};
  bool executed{false};
  std::string status{"rejected"};
  std::string reason;
  long completed_at{0};
};

struct EdgeInventoryCompartment {
  std::string compartment_id;
  std::string compartment_type;
  std::string currency;
  DenominationBreakdown denominations;
  std::string status{"ok"};
};

struct EdgeInventoryView {
  std::string device_id;
  std::vector<EdgeInventoryCompartment> compartments;
  long updated_at{0};
};

struct EdgeHealthSummary {
  std::string device_id;
  std::string device_type;
  int score{100};
  Labels factors;
  std::vector<std::string> explanations;
};

struct SimulatorState {
  std::string device_id;
  bool online{true};
  std::vector<std::string> faults;
  nlohmann::json sensor_telemetry = nlohmann::json::object();
};

struct EdgeEvent {
  std::string event_id;
  std::string tenant_id;
  std::string device_id;
  std::string event_type;
  long occurred_at{0};
  nlohmann::json metadata = nlohmann::json::object();
};

void to_json(nlohmann::json& j, const EdgeDevice& v);
void from_json(const nlohmann::json& j, EdgeDevice& v);
void to_json(nlohmann::json& j, const CapabilityMetadata& v);
void from_json(const nlohmann::json& j, CapabilityMetadata& v);
void to_json(nlohmann::json& j, const DriverMetadata& v);
void from_json(const nlohmann::json& j, DriverMetadata& v);
void to_json(nlohmann::json& j, const EdgeCommandRequest& v);
void from_json(const nlohmann::json& j, EdgeCommandRequest& v);
void to_json(nlohmann::json& j, const EdgeCommandResult& v);
void from_json(const nlohmann::json& j, EdgeCommandResult& v);
void to_json(nlohmann::json& j, const EdgeInventoryCompartment& v);
void from_json(const nlohmann::json& j, EdgeInventoryCompartment& v);
void to_json(nlohmann::json& j, const EdgeInventoryView& v);
void from_json(const nlohmann::json& j, EdgeInventoryView& v);
void to_json(nlohmann::json& j, const EdgeHealthSummary& v);
void from_json(const nlohmann::json& j, EdgeHealthSummary& v);
void to_json(nlohmann::json& j, const SimulatorState& v);
void from_json(const nlohmann::json& j, SimulatorState& v);
void to_json(nlohmann::json& j, const EdgeEvent& v);
void from_json(const nlohmann::json& j, EdgeEvent& v);

}  // namespace edge_platform
