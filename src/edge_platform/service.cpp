#include "edge_platform/service.hpp"

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>

#include "cloud/fleet_control/routes.hpp"
#include "integrations/veil/routes.hpp"
#include "obs/metrics.hpp"

namespace edge_platform {
namespace {

std::string id_for(const std::string& prefix, size_t n) {
  return prefix + "-" + std::to_string(n + 1);
}

std::set<std::string> supported_types() {
  return {"cash_drawer", "smart_safe", "cash_recycler", "teller_station",
          "kiosk", "atm_like_terminal"};
}

}  // namespace

EdgePlatformService::EdgePlatformService(std::string store_path,
                                         cloud::fleet_control::FleetControlPlane* fleet,
                                         integrations::veil::VeilTrustService* trust)
    : store_path_(std::move(store_path)), fleet_(fleet), trust_(trust) {
  load();
  std::lock_guard<std::mutex> lk(mu_);
  for (const auto& type : supported_types()) register_default_driver_locked(type);
}

bool EdgePlatformService::known_device_type(const std::string& device_type) const {
  return supported_types().count(device_type) > 0;
}

bool EdgePlatformService::register_device(EdgeDevice device) {
  if (device.device_id.empty() || device.tenant_id.empty() || !known_device_type(device.device_type)) return false;
  std::lock_guard<std::mutex> lk(mu_);
  register_default_driver_locked(device.device_type);
  if (device.driver_id.empty()) device.driver_id = "mock-" + device.device_type;
  if (device.capabilities.empty()) device.capabilities = default_capabilities_for_type(device.device_type);
  devices_[device.device_id] = device;
  std::vector<CapabilityMetadata> caps;
  for (const auto& cap : device.capabilities) {
    caps.push_back({cap, "registered edge capability", cap == "lock_control" || cap == "cash_out" ||
                                                   cap == "command_execution" || cap == "evidence_export"});
  }
  capabilities_[device.device_id] = caps;
  inventories_[device.device_id] = default_inventory_for(device);
  simulator_[device.device_id] = {device.device_id, true, {}, {{"door", "closed"}, {"lock", "locked"}}};
  if (fleet_) {
    cloud::fleet_control::DeviceRegistryRecord record;
    record.device_id = device.device_id;
    record.device_type = device.device_type;
    record.organization_id = device.tenant_id;
    record.merchant_id = device.merchant_id;
    record.branch_id = device.branch_id;
    record.region = device.region;
    record.firmware_version = device.firmware_version;
    record.enrollment_status = "enrolled";
    record.certificate_identity_status = "active";
    record.health_status = "healthy";
    record.capabilities = device.capabilities;
    record.labels = device.labels;
    fleet_->register_device(record);
  }
  emit_event_locked(device.tenant_id, device.device_id, "edge_device_registered", 0,
                    {{"device_type", device.device_type}, {"capabilities", device.capabilities}});
  emit_trust_evidence_locked(device, "edge_device_registered", 0,
                             {{"device_type", device.device_type}, {"capabilities", device.capabilities}});
  save_locked();
  return true;
}

bool EdgePlatformService::register_capability(const std::string& device_id, CapabilityMetadata capability) {
  std::lock_guard<std::mutex> lk(mu_);
  auto it = devices_.find(device_id);
  if (it == devices_.end() || capability.capability.empty()) return false;
  auto& device = it->second;
  if (std::find(device.capabilities.begin(), device.capabilities.end(), capability.capability) ==
      device.capabilities.end()) {
    device.capabilities.push_back(capability.capability);
  }
  capabilities_[device_id].push_back(capability);
  emit_event_locked(device.tenant_id, device_id, "device_capability_registered", 0,
                    {{"capability", capability.capability}});
  save_locked();
  return true;
}

std::vector<EdgeDevice> EdgePlatformService::devices(const std::string& tenant_id) const {
  std::lock_guard<std::mutex> lk(mu_);
  std::vector<EdgeDevice> out;
  for (const auto& item : devices_) {
    if (tenant_id.empty() || item.second.tenant_id == tenant_id) out.push_back(item.second);
  }
  return out;
}

std::vector<CapabilityMetadata> EdgePlatformService::capabilities(const std::string& device_id) const {
  std::lock_guard<std::mutex> lk(mu_);
  auto it = capabilities_.find(device_id);
  return it == capabilities_.end() ? std::vector<CapabilityMetadata>{} : it->second;
}

std::vector<DriverMetadata> EdgePlatformService::drivers() const {
  std::lock_guard<std::mutex> lk(mu_);
  std::vector<DriverMetadata> out;
  for (const auto& item : drivers_) out.push_back(item.second->metadata());
  return out;
}

std::string EdgePlatformService::capability_for_command(const std::string& command) const {
  if (command == "lock" || command == "unlock") return "lock_control";
  if (command == "open" || command == "close") return "open_close";
  if (command == "dispense_cash") return "cash_out";
  if (command == "accept_cash") return "cash_in";
  if (command == "count_cash" || command == "reconcile_device") return "inventory";
  if (command == "run_self_test") return "sensor_telemetry";
  if (command == "rotate_keys") return "identity";
  if (command == "export_evidence") return "evidence_export";
  if (command == "enter_maintenance_mode" || command == "exit_maintenance_mode") return "command_execution";
  return "";
}

bool EdgePlatformService::high_risk_command(const std::string& command) const {
  static const std::set<std::string> high_risk = {"lock", "unlock", "open", "dispense_cash",
                                                 "accept_cash", "rotate_keys", "export_evidence",
                                                 "enter_maintenance_mode"};
  return high_risk.count(command) > 0;
}

bool EdgePlatformService::has_capability_locked(const EdgeDevice& device, const std::string& capability) const {
  return std::find(device.capabilities.begin(), device.capabilities.end(), capability) != device.capabilities.end();
}

EdgeCommandResult EdgePlatformService::execute_command(EdgeCommandRequest request) {
  std::lock_guard<std::mutex> lk(mu_);
  if (request.command_id.empty()) request.command_id = id_for("edge-command", command_results_.size());
  EdgeCommandResult result;
  result.command_id = request.command_id;
  result.device_id = request.device_id;
  result.command = request.command;
  result.completed_at = request.requested_at;
  auto device_it = devices_.find(request.device_id);
  auto reject = [&](const std::string& reason, const std::string& event_type) {
    result.accepted = false;
    result.executed = false;
    result.status = "rejected";
    result.reason = reason;
    if (device_it != devices_.end()) emit_event_locked(device_it->second.tenant_id, request.device_id, event_type,
                                                       request.requested_at, {{"command", request.command}});
    command_results_.push_back(result);
    ++command_count_[request.device_id];
    ++command_failures_[request.device_id];
    save_locked();
    return result;
  };
  if (device_it == devices_.end()) return reject("unknown_device", "driver_command_failed");
  auto& device = device_it->second;
  auto capability = capability_for_command(request.command);
  if (capability.empty()) {
    emit_trust_evidence_locked(device, "unsupported_command_rejected", request.requested_at,
                               {{"command", request.command}, {"reason", "unknown_command"}});
    return reject("unsupported_command", "unsupported_command_rejected");
  }
  if (!has_capability_locked(device, capability)) {
    emit_trust_evidence_locked(device, "unsupported_command_rejected", request.requested_at,
                               {{"command", request.command}, {"required_capability", capability}});
    return reject("missing_capability:" + capability, "unsupported_command_rejected");
  }
  if (!simulator_[request.device_id].online) return reject("device_offline", "driver_command_failed");
  if (high_risk_command(request.command) && trust_) {
    bool allowed = trust_->policy_allows_or_fail_closed(device.tenant_id, device.device_id, request.command,
                                                        {{"device_type", device.device_type},
                                                         {"capability", capability}},
                                                        request.requested_at);
    if (!allowed) {
      emit_trust_evidence_locked(device, "edge_policy_denied_command", request.requested_at,
                                 {{"command", request.command}});
      return reject("policy_denied", "driver_command_failed");
    }
  }
  auto driver_it = drivers_.find(device.driver_id);
  if (driver_it == drivers_.end()) return reject("driver_missing", "driver_command_failed");
  result = driver_it->second->execute(device, request);
  ++command_count_[request.device_id];
  if (!result.executed) ++command_failures_[request.device_id];
  command_results_.push_back(result);
  emit_event_locked(device.tenant_id, device.device_id,
                    result.executed ? "driver_command_executed" : "driver_command_failed",
                    request.requested_at, {{"command", request.command}, {"status", result.status}});
  if (request.command == "enter_maintenance_mode") {
    simulator_[request.device_id].sensor_telemetry["maintenance_mode"] = true;
    emit_event_locked(device.tenant_id, device.device_id, "maintenance_mode_entered", request.requested_at);
  }
  if (request.command == "exit_maintenance_mode") {
    simulator_[request.device_id].sensor_telemetry["maintenance_mode"] = false;
    emit_event_locked(device.tenant_id, device.device_id, "maintenance_mode_exited", request.requested_at);
  }
  if (high_risk_command(request.command)) {
    emit_trust_evidence_locked(device, "edge_command_executed", request.requested_at,
                               {{"command", request.command}, {"capability", capability}, {"result", result}});
  }
  save_locked();
  return result;
}

EdgeInventoryView EdgePlatformService::inventory(const std::string& device_id) const {
  std::lock_guard<std::mutex> lk(mu_);
  auto it = inventories_.find(device_id);
  return it == inventories_.end() ? EdgeInventoryView{device_id, {}, 0} : it->second;
}

bool EdgePlatformService::update_inventory(EdgeInventoryView inventory) {
  std::lock_guard<std::mutex> lk(mu_);
  auto it = devices_.find(inventory.device_id);
  if (it == devices_.end()) return false;
  inventories_[inventory.device_id] = inventory;
  emit_event_locked(it->second.tenant_id, inventory.device_id, "edge_inventory_updated",
                    inventory.updated_at, {{"compartments", inventory.compartments.size()}});
  emit_trust_evidence_locked(it->second, "edge_inventory_updated", inventory.updated_at, inventory);
  save_locked();
  return true;
}

SimulatorState EdgePlatformService::simulate(const std::string& device_id, const std::string& action,
                                             const nlohmann::json& parameters, long now_epoch) {
  std::lock_guard<std::mutex> lk(mu_);
  auto dev = devices_.find(device_id);
  if (dev == devices_.end()) return {device_id, false, {"unknown_device"}, {}};
  auto& state = simulator_[device_id];
  if (action == "offline") state.online = false;
  if (action == "online") state.online = true;
  if (action == "fault") {
    auto fault = parameters.value("fault", "simulated_fault");
    state.faults.push_back(fault);
    emit_event_locked(dev->second.tenant_id, device_id, "device_fault_detected", now_epoch, {{"fault", fault}});
  }
  if (action == "telemetry") state.sensor_telemetry = parameters;
  if (action == "cash_in") {
    auto inv = inventories_[device_id];
    if (!inv.compartments.empty()) inv.compartments.front().denominations["100"] += parameters.value("quantity", 1);
    inv.updated_at = now_epoch;
    inventories_[device_id] = inv;
    emit_event_locked(dev->second.tenant_id, device_id, "edge_inventory_updated", now_epoch, {{"simulated", true}});
  }
  emit_event_locked(dev->second.tenant_id, device_id, "simulator_event_generated", now_epoch,
                    {{"action", action}, {"parameters", parameters}});
  save_locked();
  return state;
}

EdgeHealthSummary EdgePlatformService::health(const std::string& device_id) {
  std::lock_guard<std::mutex> lk(mu_);
  auto summary = health_locked(device_id);
  obs::M().gauge("register_edge_device_health", "Financial edge device health",
                 {{"device_id", device_id}, {"device_type", summary.device_type}})
      .set(summary.score);
  save_locked();
  return summary;
}

EdgeHealthSummary EdgePlatformService::health_locked(const std::string& device_id) {
  EdgeHealthSummary h;
  h.device_id = device_id;
  auto dev = devices_.find(device_id);
  if (dev == devices_.end()) {
    h.score = 0;
    h.explanations.push_back("unknown device");
    return h;
  }
  h.device_type = dev->second.device_type;
  int penalty = 0;
  auto sim = simulator_[device_id];
  if (!sim.online) {
    h.factors["sensor_health"] = "offline";
    h.explanations.push_back("device simulator is offline");
    penalty += 25;
  } else {
    h.factors["sensor_health"] = "online";
  }
  if (!sim.faults.empty()) {
    h.factors["cash_jam_fault_indicators"] = std::to_string(sim.faults.size());
    h.explanations.push_back("simulated faults reduced score");
    penalty += static_cast<int>(sim.faults.size()) * 15;
  }
  auto inv = inventories_[device_id];
  int bad_compartments = 0;
  for (const auto& c : inv.compartments) {
    if (c.status != "ok") ++bad_compartments;
  }
  h.factors["compartment_status"] = std::to_string(bad_compartments);
  penalty += bad_compartments * 10;
  h.factors["lock_health"] = sim.sensor_telemetry.value("lock", "unknown");
  if (sim.sensor_telemetry.value("maintenance_mode", false)) {
    h.factors["maintenance_state"] = "maintenance";
    penalty += 10;
  } else {
    h.factors["maintenance_state"] = "normal";
  }
  int total = command_count_[device_id];
  int failed = command_failures_[device_id];
  h.factors["command_failure_rate"] = total ? std::to_string(failed) + "/" + std::to_string(total) : "0/0";
  penalty += total ? (failed * 20 / total) : 0;
  if (trust_) {
    integrations::veil::EvidenceFilter filter;
    filter.tenant_id = dev->second.tenant_id;
    filter.device_id = device_id;
    auto verification = trust_->verify_chain(filter);
    h.factors["evidence_chain_health"] = verification.valid ? "valid" : "invalid";
    if (!verification.valid) penalty += 20;
  } else {
    h.factors["evidence_chain_health"] = "unavailable";
  }
  h.score = std::max(0, 100 - penalty);
  return h;
}

std::vector<EdgeEvent> EdgePlatformService::events(const std::string& tenant_id) const {
  std::lock_guard<std::mutex> lk(mu_);
  std::vector<EdgeEvent> out;
  for (const auto& e : events_) {
    if (tenant_id.empty() || e.tenant_id == tenant_id) out.push_back(e);
  }
  return out;
}

EdgeInventoryView EdgePlatformService::default_inventory_for(const EdgeDevice& device) const {
  std::string compartment = "drawer";
  if (device.device_type == "smart_safe") compartment = "vault_section";
  if (device.device_type == "cash_recycler") compartment = "recycler_bin";
  if (device.device_type == "teller_station") compartment = "teller_drawer";
  if (device.device_type == "kiosk" || device.device_type == "atm_like_terminal") compartment = "terminal_cash_slot";
  return {device.device_id, {{"main", compartment, "USD", {{"100", 10}, {"500", 5}}, "ok"}}, 0};
}

void EdgePlatformService::register_default_driver_locked(const std::string& device_type) {
  auto id = "mock-" + device_type;
  if (drivers_.count(id) == 0) drivers_[id] = make_mock_driver(device_type);
}

void EdgePlatformService::emit_event_locked(const std::string& tenant_id, const std::string& device_id,
                                            const std::string& event_type, long now_epoch,
                                            nlohmann::json metadata) {
  events_.push_back({id_for("edge-event", events_.size()), tenant_id, device_id, event_type,
                     now_epoch, std::move(metadata)});
}

void EdgePlatformService::emit_trust_evidence_locked(const EdgeDevice& device, const std::string& event_type,
                                                     long now_epoch, nlohmann::json payload) {
  if (!trust_) return;
  integrations::veil::TrustEvidenceRecord evidence;
  evidence.tenant_id = device.tenant_id;
  evidence.device_id = device.device_id;
  evidence.merchant_id = device.merchant_id;
  evidence.branch_id = device.branch_id;
  evidence.region = device.region;
  evidence.event_type = event_type;
  evidence.source_module = "edge_platform";
  evidence.timestamp = now_epoch;
  evidence.policy_context = {{"device_type", device.device_type}, {"capabilities", device.capabilities}};
  evidence.classification_labels = {"financial_edge_device"};
  evidence.sensitivity_labels = {"cash_ops", "device_control"};
  evidence.audit_event_id = id_for("edge-audit", events_.size());
  evidence.correlation_id = device.device_id + ":" + event_type + ":" + std::to_string(now_epoch);
  evidence.payload = std::move(payload);
  trust_->create_evidence(evidence);
}

void EdgePlatformService::save() const {
  std::lock_guard<std::mutex> lk(mu_);
  save_locked();
}

void EdgePlatformService::load() {
  if (store_path_.empty()) return;
  std::ifstream in(store_path_);
  if (!in) return;
  nlohmann::json j;
  in >> j;
  for (const auto& item : j.value("devices", nlohmann::json::array())) {
    auto d = item.get<EdgeDevice>();
    devices_[d.device_id] = d;
  }
  for (const auto& item : j.value("capabilities", nlohmann::json::object()).items()) {
    capabilities_[item.key()] = item.value().get<std::vector<CapabilityMetadata>>();
  }
  for (const auto& item : j.value("inventories", nlohmann::json::array())) {
    auto inv = item.get<EdgeInventoryView>();
    inventories_[inv.device_id] = inv;
  }
  for (const auto& item : j.value("simulator", nlohmann::json::array())) {
    auto state = item.get<SimulatorState>();
    simulator_[state.device_id] = state;
  }
  command_results_ = j.value("command_results", command_results_);
  events_ = j.value("events", events_);
}

void EdgePlatformService::save_locked() const {
  if (store_path_.empty()) return;
  auto parent = std::filesystem::path(store_path_).parent_path();
  if (!parent.empty()) std::filesystem::create_directories(parent);
  nlohmann::json devices = nlohmann::json::array();
  for (const auto& item : devices_) devices.push_back(item.second);
  nlohmann::json caps = nlohmann::json::object();
  for (const auto& item : capabilities_) caps[item.first] = item.second;
  nlohmann::json inventories = nlohmann::json::array();
  for (const auto& item : inventories_) inventories.push_back(item.second);
  nlohmann::json simulator = nlohmann::json::array();
  for (const auto& item : simulator_) simulator.push_back(item.second);
  std::ofstream(store_path_, std::ios::trunc)
      << nlohmann::json{{"schema", 1},
                        {"devices", devices},
                        {"capabilities", caps},
                        {"inventories", inventories},
                        {"simulator", simulator},
                        {"command_results", command_results_},
                        {"events", events_}}
             .dump(2)
      << "\n";
}

EdgePlatformService& default_edge_platform() {
  static EdgePlatformService* svc = [] {
    const char* env = std::getenv("REGISTER_MVP_EDGE_STORE");
    std::string path = env && *env ? std::string(env) : "data/edge_platform.json";
    return new EdgePlatformService(path, &cloud::fleet_control::default_control_plane(),
                                   &integrations::veil::default_trust_service());
  }();
  return *svc;
}

}  // namespace edge_platform
