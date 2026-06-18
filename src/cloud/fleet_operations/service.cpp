#include "cloud/fleet_operations/service.hpp"

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <set>

#include "integrations/veil/models.hpp"
namespace cloud::fleet_operations {
namespace {

std::string id_for(const std::string& prefix, size_t n) {
  return prefix + "-" + std::to_string(n + 1);
}

std::string epoch_string(long epoch) {
  return std::to_string(epoch);
}

std::string risk_level(int score) {
  if (score >= 75) return "critical";
  if (score >= 50) return "high";
  if (score >= 25) return "medium";
  return "low";
}

template <typename T>
nlohmann::json map_values(const std::unordered_map<std::string, T>& values) {
  auto out = nlohmann::json::array();
  for (const auto& item : values) out.push_back(item.second);
  return out;
}

}  // namespace

FleetOperationsService::FleetOperationsService(std::string store_path,
                                               edge_platform::EdgePlatformService* edge,
                                               integrations::veil::VeilTrustService* trust,
                                               long heartbeat_timeout_seconds)
    : store_path_(std::move(store_path)),
      edge_(edge),
      trust_(trust),
      heartbeat_timeout_seconds_(heartbeat_timeout_seconds) {
  load();
}

bool FleetOperationsService::valid_transition(const std::string& from, const std::string& to,
                                              bool recovery) const {
  if (from == to) return true;
  if (from == "retired") return false;
  if (to == "retired" || to == "quarantined") return true;
  if (from == "provisioned") return to == "active";
  if (from == "active") return to == "maintenance";
  if (from == "maintenance") return to == "active";
  if (from == "quarantined") return recovery && to == "active";
  return false;
}

bool FleetOperationsService::enroll(FleetDevice device, const std::string& actor_id,
                                    long now_epoch, std::string* error) {
  auto fail = [&](const std::string& value) {
    if (error) *error = value;
    return false;
  };
  if (device.device_id.empty() || device.tenant_id.empty() || device.location_id.empty() ||
      device.branch_id.empty() || device.fleet_id.empty()) {
    return fail("missing_required_assignment");
  }
  if (device.lifecycle_state != "provisioned" && device.lifecycle_state != "active") {
    return fail("invalid_initial_lifecycle");
  }
  std::lock_guard<std::mutex> lk(mu_);
  if (devices_.count(device.device_id)) return fail("device_already_enrolled");

  if (edge_) {
    edge_platform::EdgeDevice edge_device;
    edge_device.device_id = device.device_id;
    edge_device.tenant_id = device.tenant_id;
    edge_device.merchant_id = device.merchant_id;
    edge_device.branch_id = device.branch_id;
    edge_device.region = device.region;
    edge_device.device_type = device.device_type;
    edge_device.driver_id = device.driver_id;
    edge_device.firmware_version = device.firmware_version;
    edge_device.capabilities = device.capabilities;
    edge_device.labels = device.labels;
    auto existing = edge_->devices(device.tenant_id);
    bool found = std::any_of(existing.begin(), existing.end(), [&](const auto& v) {
      return v.device_id == device.device_id;
    });
    if (!found && !edge_->register_device(edge_device)) return fail("edge_registration_failed");
    if (device.capabilities.empty()) {
      existing = edge_->devices(device.tenant_id);
      auto it = std::find_if(existing.begin(), existing.end(), [&](const auto& v) {
        return v.device_id == device.device_id;
      });
      if (it != existing.end()) {
        device.capabilities = it->capabilities;
        device.driver_id = it->driver_id;
      }
    }
  }
  if (device.capabilities.empty()) return fail("unknown_capability_state");
  device.connectivity_status = "offline";
  devices_[device.device_id] = device;
  locations_[device.location_id] = {device.location_id, device.tenant_id, device.branch_id,
                                    device.region, device.location_id, device.labels};
  fleets_[device.fleet_id] = {device.fleet_id, device.tenant_id, device.fleet_id, {}, {}};
  if (!device.group_id.empty()) {
    auto& group = groups_[device.group_id];
    group.group_id = device.group_id;
    group.tenant_id = device.tenant_id;
    group.fleet_id = device.fleet_id;
    group.name = device.group_id;
    group.device_ids.push_back(device.device_id);
  }
  enrollments_[device.device_id] = {id_for("enrollment", enrollments_.size()), device.tenant_id,
                                     device.device_id, epoch_string(now_epoch), actor_id, "enrolled"};
  assignments_[device.device_id] = {device.tenant_id, device.device_id, device.fleet_id,
                                     device.group_id, device.location_id, device.branch_id,
                                     epoch_string(now_epoch)};
  append_event_locked(device.tenant_id, device.device_id, "device_enrolled", now_epoch,
                      {{"location_id", device.location_id}, {"fleet_id", device.fleet_id},
                       {"lifecycle_state", device.lifecycle_state}});
  emit_evidence_locked(device, "device_enrollment", now_epoch,
                       {{"location_id", device.location_id}, {"fleet_id", device.fleet_id}});
  save_locked();
  return true;
}

std::optional<FleetDevice> FleetOperationsService::device(const std::string& tenant_id,
                                                          const std::string& device_id) const {
  std::lock_guard<std::mutex> lk(mu_);
  auto it = devices_.find(device_id);
  if (it == devices_.end() || it->second.tenant_id != tenant_id) return std::nullopt;
  return it->second;
}

std::vector<FleetDevice> FleetOperationsService::devices(const std::string& tenant_id) const {
  std::lock_guard<std::mutex> lk(mu_);
  std::vector<FleetDevice> out;
  for (const auto& item : devices_) {
    if (item.second.tenant_id == tenant_id) out.push_back(item.second);
  }
  std::sort(out.begin(), out.end(), [](const auto& a, const auto& b) {
    return a.device_id < b.device_id;
  });
  return out;
}

bool FleetOperationsService::update_device(const std::string& tenant_id,
                                           const std::string& device_id,
                                           const nlohmann::json& patch, long now_epoch,
                                           std::string* error) {
  std::lock_guard<std::mutex> lk(mu_);
  auto it = devices_.find(device_id);
  if (it == devices_.end() || it->second.tenant_id != tenant_id) {
    if (error) *error = "not_found";
    return false;
  }
  auto& d = it->second;
  if (d.retired) {
    if (error) *error = "device_retired";
    return false;
  }
  auto next = patch.value("lifecycle_state", d.lifecycle_state);
  if (!valid_transition(d.lifecycle_state, next, false)) {
    if (error) *error = "invalid_lifecycle_transition";
    return false;
  }
  if (patch.contains("location_id") && patch.value("location_id", "").empty()) {
    if (error) *error = "location_required";
    return false;
  }
  auto prior = d.lifecycle_state;
  d.lifecycle_state = next;
  d.location_id = patch.value("location_id", d.location_id);
  d.branch_id = patch.value("branch_id", d.branch_id);
  d.group_id = patch.value("group_id", d.group_id);
  d.fleet_id = patch.value("fleet_id", d.fleet_id);
  d.labels = patch.value("labels", d.labels);
  if (prior != next) {
    append_event_locked(tenant_id, device_id, "device_lifecycle_changed", now_epoch,
                        {{"from", prior}, {"to", next}});
  }
  assignments_[device_id] = {tenant_id, device_id, d.fleet_id, d.group_id, d.location_id,
                              d.branch_id, epoch_string(now_epoch)};
  save_locked();
  return true;
}

bool FleetOperationsService::retire_device(const std::string& tenant_id,
                                           const std::string& device_id, long now_epoch,
                                           std::string* error) {
  std::lock_guard<std::mutex> lk(mu_);
  auto it = devices_.find(device_id);
  if (it == devices_.end() || it->second.tenant_id != tenant_id) {
    if (error) *error = "not_found";
    return false;
  }
  if (it->second.retired) {
    if (error) *error = "device_already_retired";
    return false;
  }
  auto prior = it->second.lifecycle_state;
  it->second.lifecycle_state = "retired";
  it->second.retired = true;
  it->second.connectivity_status = "offline";
  append_event_locked(tenant_id, device_id, "device_retired", now_epoch,
                      {{"from", prior}, {"irreversible", true}});
  emit_evidence_locked(it->second, "device_retired", now_epoch, {{"from", prior}});
  save_locked();
  return true;
}

bool FleetOperationsService::receive_heartbeat(FleetHeartbeat heartbeat, long received_at,
                                               std::string* error) {
  std::lock_guard<std::mutex> lk(mu_);
  auto it = devices_.find(heartbeat.device_id);
  if (it == devices_.end()) {
    if (error) *error = "unknown_device";
    return false;
  }
  auto& d = it->second;
  if (heartbeat.tenant_id != d.tenant_id) {
    quarantine_locked(d, "tenant_boundary_violation", received_at);
    if (error) *error = "tenant_mismatch";
    save_locked();
    return false;
  }
  if (d.retired) {
    if (error) *error = "device_retired";
    return false;
  }
  if (heartbeat.timestamp <= 0 || received_at - heartbeat.timestamp > heartbeat_timeout_seconds_ ||
      heartbeat.timestamp > received_at + 30) {
    quarantine_locked(d, "stale_heartbeat", received_at);
    if (error) *error = "stale_heartbeat";
    save_locked();
    return false;
  }
  if (d.lifecycle_state == "quarantined") {
    heartbeats_[d.device_id] = heartbeat;
    d.last_heartbeat_epoch = heartbeat.timestamp;
    d.last_heartbeat_at = epoch_string(heartbeat.timestamp);
    d.connectivity_status = heartbeat.connectivity_status;
    append_event_locked(d.tenant_id, d.device_id, "heartbeat_received", received_at,
                        {{"quarantined", true}});
    if (error) *error = "device_quarantined";
    save_locked();
    return false;
  }
  if (!capability_known_locked(d)) {
    quarantine_locked(d, "unknown_capability_state", received_at);
    if (error) *error = "unknown_capability_state";
    save_locked();
    return false;
  }
  if (heartbeat.firmware_version.empty() || heartbeat.firmware_version == "unknown") {
    quarantine_locked(d, "unknown_firmware", received_at);
    if (error) *error = "unknown_firmware";
    save_locked();
    return false;
  }
  if (heartbeat.driver_version.empty() || heartbeat.driver_version == "unknown") {
    quarantine_locked(d, "unknown_driver", received_at);
    if (error) *error = "unknown_driver";
    save_locked();
    return false;
  }
  if (heartbeat.inventory_summary.value("contradiction", false)) {
    quarantine_locked(d, "inventory_contradiction", received_at);
    if (error) *error = "inventory_contradiction";
    save_locked();
    return false;
  }
  heartbeats_[d.device_id] = heartbeat;
  d.firmware_version = heartbeat.firmware_version;
  d.driver_version = heartbeat.driver_version;
  d.connectivity_status = heartbeat.connectivity_status;
  d.last_heartbeat_epoch = heartbeat.timestamp;
  d.last_heartbeat_at = epoch_string(heartbeat.timestamp);
  append_event_locked(d.tenant_id, d.device_id, "heartbeat_received", received_at,
                      {{"status", heartbeat.connectivity_status},
                       {"error_codes", heartbeat.error_codes}});
  emit_evidence_locked(d, "device_heartbeat", heartbeat.timestamp,
                       {{"status", heartbeat.connectivity_status},
                        {"health_metrics", heartbeat.health_metrics}});
  save_locked();
  return true;
}

nlohmann::json FleetOperationsService::freshness(const std::string& tenant_id,
                                                 const std::string& device_id,
                                                 long now_epoch) const {
  std::lock_guard<std::mutex> lk(mu_);
  auto it = devices_.find(device_id);
  if (it == devices_.end() || it->second.tenant_id != tenant_id) {
    return {{"found", false}};
  }
  long age = it->second.last_heartbeat_epoch ? now_epoch - it->second.last_heartbeat_epoch : -1;
  return {{"found", true}, {"device_id", device_id}, {"fresh", heartbeat_fresh_locked(it->second, now_epoch)},
          {"age_seconds", age}, {"timeout_seconds", heartbeat_timeout_seconds_},
          {"last_heartbeat_at", it->second.last_heartbeat_at}};
}

nlohmann::json FleetOperationsService::health(const std::string& tenant_id,
                                              const std::string& device_id,
                                              long now_epoch) const {
  std::lock_guard<std::mutex> lk(mu_);
  auto it = devices_.find(device_id);
  if (it == devices_.end() || it->second.tenant_id != tenant_id) return {{"found", false}};
  auto hb = heartbeats_.find(device_id);
  return {{"found", true}, {"device_id", device_id},
          {"lifecycle_state", it->second.lifecycle_state},
          {"connectivity_status", it->second.connectivity_status},
          {"fresh", heartbeat_fresh_locked(it->second, now_epoch)},
          {"health_metrics", hb == heartbeats_.end() ? nlohmann::json::object() : hb->second.health_metrics},
          {"error_codes", hb == heartbeats_.end() ? nlohmann::json::array() : nlohmann::json(hb->second.error_codes)}};
}

std::string FleetOperationsService::capability_for_command(const std::string& command) const {
  if (command == "lock" || command == "unlock") return "lock_control";
  if (command == "open" || command == "close") return "open_close";
  if (command == "dispense_cash") return "cash_out";
  if (command == "accept_cash") return "cash_in";
  if (command == "count_cash" || command == "reconcile_device" || command == "sync_inventory") return "inventory";
  if (command == "run_self_test") return "sensor_telemetry";
  if (command == "rotate_keys") return "identity";
  if (command == "export_evidence") return "evidence_export";
  if (command == "restart" || command == "refresh_config" ||
      command == "pause_transactions" || command == "resume_transactions" ||
      command == "disable_device" || command == "enable_device" ||
      command == "enter_maintenance_mode" || command == "exit_maintenance_mode") {
    return "command_execution";
  }
  return "";
}

bool FleetOperationsService::high_risk_command(const std::string& command) const {
  static const std::set<std::string> commands{
      "lock", "unlock", "open", "dispense_cash", "accept_cash", "rotate_keys",
      "export_evidence", "disable_device", "enable_device", "pause_transactions",
      "resume_transactions", "enter_maintenance_mode"};
  return commands.count(command) > 0;
}

FleetCommand FleetOperationsService::create_command(FleetCommand command, long now_epoch) {
  std::lock_guard<std::mutex> lk(mu_);
  command.command_id = command.command_id.empty() ? id_for("fleet-command", commands_.size())
                                                  : command.command_id;
  command.created_at = now_epoch;
  if (command.expires_at <= now_epoch) command.expires_at = now_epoch + 60;
  if (commands_.count(command.command_id)) {
    command.state = "failed";
    command.failure_reason = "duplicate_command_id";
    append_event_locked(command.tenant_id, command.device_id, "command_denied", now_epoch,
                        {{"command_id", command.command_id},
                         {"reason", "duplicate_command_id"}});
    save_locked();
    return command;
  }
  command.state = "queued";
  append_event_locked(command.tenant_id, command.device_id, "command_queued", now_epoch,
                      {{"command_id", command.command_id}, {"command", command.command}});
  auto fail = [&](const std::string& reason, bool policy_denied = false) {
    command.state = "failed";
    command.failure_reason = reason;
    commands_[command.command_id] = command;
    append_event_locked(command.tenant_id, command.device_id,
                        policy_denied ? "command_denied" : "command_failed", now_epoch,
                        {{"command_id", command.command_id}, {"reason", reason}});
    save_locked();
    return command;
  };
  auto device_it = devices_.find(command.device_id);
  if (device_it == devices_.end() || device_it->second.tenant_id != command.tenant_id) {
    return fail("tenant_or_device_mismatch");
  }
  auto& device = device_it->second;
  if (device.retired || device.lifecycle_state == "quarantined") return fail("device_unavailable");
  if (!heartbeat_fresh_locked(device, now_epoch)) return fail("stale_heartbeat");
  if (device.connectivity_status != "online") return fail("device_offline");
  if (!capability_known_locked(device)) return fail("unknown_capability_state");
  command.required_capability = capability_for_command(command.command);
  if (command.required_capability.empty() ||
      std::find(device.capabilities.begin(), device.capabilities.end(), command.required_capability) ==
          device.capabilities.end()) {
    return fail("unsupported_capability");
  }
  command.state = "policy_checking";
  command.high_risk = high_risk_command(command.command);
  std::string policy_action = command.high_risk ? command.command : "command_execution";
  if (!trust_ || !trust_->policy_allows_or_fail_closed(
                     command.tenant_id, command.device_id, policy_action,
                     {{"command", command.command}, {"parameters", command.parameters},
                      {"required_capability", command.required_capability}},
                     now_epoch)) {
    ++policy_violations_[command.device_id];
    quarantine_locked(device, "failed_veil_trust_check", now_epoch);
    return fail("policy_denied", true);
  }
  if (command.high_risk && (command.approval_id.empty() || command.approved_by.empty())) {
    command.failure_reason = "approval_required";
    commands_[command.command_id] = command;
    save_locked();
    return command;
  }
  command.state = "approved";
  append_event_locked(command.tenant_id, command.device_id, "command_approved", now_epoch,
                      {{"command_id", command.command_id}, {"approval_id", command.approval_id}});
  command.state = "dispatched";
  command.dispatched_at = now_epoch;
  commands_[command.command_id] = command;
  append_event_locked(command.tenant_id, command.device_id, "command_dispatched", now_epoch,
                      {{"command_id", command.command_id}});
  emit_evidence_locked(device, "command_delivery", now_epoch, command);
  save_locked();
  return command;
}

std::optional<FleetCommand> FleetOperationsService::command(const std::string& tenant_id,
                                                            const std::string& command_id) const {
  std::lock_guard<std::mutex> lk(mu_);
  auto it = commands_.find(command_id);
  if (it == commands_.end() || it->second.tenant_id != tenant_id) return std::nullopt;
  return it->second;
}

std::vector<FleetCommand> FleetOperationsService::commands(const std::string& tenant_id) const {
  std::lock_guard<std::mutex> lk(mu_);
  std::vector<FleetCommand> out;
  for (const auto& item : commands_) {
    if (item.second.tenant_id == tenant_id) out.push_back(item.second);
  }
  std::sort(out.begin(), out.end(), [](const auto& a, const auto& b) {
    return a.command_id < b.command_id;
  });
  return out;
}

bool FleetOperationsService::cancel_command(const std::string& tenant_id,
                                            const std::string& command_id, long now_epoch,
                                            std::string* error) {
  std::lock_guard<std::mutex> lk(mu_);
  auto it = commands_.find(command_id);
  if (it == commands_.end() || it->second.tenant_id != tenant_id) {
    if (error) *error = "not_found";
    return false;
  }
  static const std::set<std::string> cancellable{"queued", "policy_checking", "approved", "dispatched"};
  if (!cancellable.count(it->second.state)) {
    if (error) *error = "command_not_cancellable";
    return false;
  }
  it->second.state = "cancelled";
  append_event_locked(tenant_id, it->second.device_id, "command_cancelled", now_epoch,
                      {{"command_id", command_id}});
  save_locked();
  return true;
}

bool FleetOperationsService::acknowledge_command(const std::string& tenant_id,
                                                 const std::string& command_id,
                                                 long now_epoch, std::string* error) {
  std::lock_guard<std::mutex> lk(mu_);
  auto it = commands_.find(command_id);
  if (it == commands_.end() || it->second.tenant_id != tenant_id) {
    if (error) *error = "not_found";
    return false;
  }
  if (it->second.state != "dispatched") {
    if (error) *error = "invalid_command_transition";
    return false;
  }
  if (now_epoch > it->second.expires_at) {
    it->second.state = "expired";
    it->second.failure_reason = "expired";
    append_event_locked(tenant_id, it->second.device_id, "command_failed", now_epoch,
                        {{"command_id", command_id}, {"reason", "expired"}});
    save_locked();
    if (error) *error = "command_expired";
    return false;
  }
  it->second.state = "acknowledged";
  it->second.acknowledged_at = now_epoch;
  append_event_locked(tenant_id, it->second.device_id, "command_acknowledged", now_epoch,
                      {{"command_id", command_id}});
  save_locked();
  return true;
}

bool FleetOperationsService::complete_command(const std::string& tenant_id,
                                              const std::string& command_id, bool success,
                                              const std::string& result, long now_epoch,
                                              std::string* error) {
  std::lock_guard<std::mutex> lk(mu_);
  auto it = commands_.find(command_id);
  if (it == commands_.end() || it->second.tenant_id != tenant_id) {
    if (error) *error = "not_found";
    return false;
  }
  if (it->second.state != "acknowledged") {
    if (error) *error = "invalid_command_transition";
    return false;
  }
  it->second.state = success ? "completed" : "failed";
  it->second.completed_at = now_epoch;
  it->second.result = result;
  if (!success) it->second.failure_reason = result.empty() ? "device_reported_failure" : result;
  append_event_locked(tenant_id, it->second.device_id,
                      success ? "command_completed" : "command_failed", now_epoch,
                      {{"command_id", command_id}, {"result", result}});
  auto device_it = devices_.find(it->second.device_id);
  if (device_it != devices_.end()) {
    emit_evidence_locked(device_it->second, success ? "command_completed" : "command_failed",
                         now_epoch, it->second);
    if (!success) {
      if (result.find("suspicious") != std::string::npos)
        quarantine_locked(device_it->second, "suspicious_command_result", now_epoch);
      int failures = 0;
      for (const auto& item : commands_) {
        if (item.second.device_id == it->second.device_id && item.second.state == "failed") ++failures;
      }
      if (failures >= 3) quarantine_locked(device_it->second, "repeated_command_failures", now_epoch);
    }
  }
  save_locked();
  return true;
}

int FleetOperationsService::expire_commands(long now_epoch) {
  std::lock_guard<std::mutex> lk(mu_);
  int count = 0;
  for (auto& item : commands_) {
    auto& c = item.second;
    if ((c.state == "queued" || c.state == "policy_checking" || c.state == "approved" ||
         c.state == "dispatched") &&
        now_epoch > c.expires_at) {
      c.state = "expired";
      c.failure_reason = "expired";
      append_event_locked(c.tenant_id, c.device_id, "command_failed", now_epoch,
                          {{"command_id", c.command_id}, {"reason", "expired"}});
      ++count;
    }
  }
  save_locked();
  return count;
}

bool FleetOperationsService::heartbeat_fresh_locked(const FleetDevice& device,
                                                    long now_epoch) const {
  return device.last_heartbeat_epoch > 0 && now_epoch >= device.last_heartbeat_epoch &&
         now_epoch - device.last_heartbeat_epoch <= heartbeat_timeout_seconds_;
}

bool FleetOperationsService::capability_known_locked(const FleetDevice& device) const {
  return !device.capabilities.empty();
}

FleetRisk FleetOperationsService::device_risk_locked(const FleetDevice& device,
                                                     long now_epoch) const {
  FleetRisk risk;
  risk.tenant_id = device.tenant_id;
  risk.scope_id = device.device_id;
  risk.evaluated_at = now_epoch;
  auto add = [&](const std::string& factor, int points, const std::string& explanation) {
    risk.score += points;
    risk.factors.push_back({factor, points, explanation});
  };
  if (!heartbeat_fresh_locked(device, now_epoch)) add("stale_heartbeat", 30, "heartbeat exceeds freshness threshold");
  int total = 0;
  int failures = 0;
  for (const auto& item : commands_) {
    if (item.second.device_id != device.device_id) continue;
    ++total;
    if (item.second.state == "failed" || item.second.state == "expired") ++failures;
  }
  if (failures >= 3) add("repeated_command_failures", 25, "three or more commands failed");
  if (total > 0 && failures * 2 >= total) add("command_failure_rate", 15, "at least half of commands failed");
  auto hb = heartbeats_.find(device.device_id);
  if (hb != heartbeats_.end()) {
    if (hb->second.inventory_summary.value("drift", 0.0) > 0.10)
      add("inventory_drift", 20, "reported inventory drift exceeds ten percent");
    if (hb->second.health_metrics.value("unusual_cash_movement", false))
      add("unusual_cash_movement", 25, "heartbeat reports unusual cash movement");
    if (hb->second.health_metrics.value("missing_trust_evidence", false))
      add("missing_trust_evidence", 20, "required trust evidence is missing");
    if (hb->second.health_metrics.value("driver_stability", 1.0) < 0.75)
      add("unstable_vendor_driver", 20, "driver stability is below threshold");
  }
  auto violations = policy_violations_.find(device.device_id);
  if (violations != policy_violations_.end() && violations->second > 0)
    add("policy_violations", std::min(30, violations->second * 10), "VEIL policy checks were denied");
  if (device.lifecycle_state == "quarantined") add("quarantined", 20, device.quarantine_reason);
  risk.score = std::min(100, risk.score);
  risk.level = risk_level(risk.score);
  return risk;
}

FleetRisk FleetOperationsService::device_risk(const std::string& tenant_id,
                                              const std::string& device_id,
                                              long now_epoch) {
  std::lock_guard<std::mutex> lk(mu_);
  auto it = devices_.find(device_id);
  if (it == devices_.end() || it->second.tenant_id != tenant_id) {
    return {tenant_id, "device", device_id, 100, "critical",
            {{"unknown_device", 100, "device is not present in tenant registry"}}, now_epoch};
  }
  auto risk = device_risk_locked(it->second, now_epoch);
  record_risk_change_locked(risk, now_epoch);
  save_locked();
  return risk;
}

std::vector<FleetRisk> FleetOperationsService::fleet_risk(const std::string& tenant_id,
                                                          long now_epoch) {
  std::lock_guard<std::mutex> lk(mu_);
  std::vector<FleetRisk> out;
  for (const auto& item : devices_) {
    if (item.second.tenant_id == tenant_id) {
      auto risk = device_risk_locked(item.second, now_epoch);
      record_risk_change_locked(risk, now_epoch);
      out.push_back(risk);
    }
  }
  std::sort(out.begin(), out.end(), [](const auto& a, const auto& b) {
    return a.scope_id < b.scope_id;
  });
  save_locked();
  return out;
}

FleetRisk FleetOperationsService::location_risk(const std::string& tenant_id,
                                                const std::string& location_id,
                                                long now_epoch) {
  std::lock_guard<std::mutex> lk(mu_);
  FleetRisk aggregate;
  aggregate.tenant_id = tenant_id;
  aggregate.scope_type = "location";
  aggregate.scope_id = location_id;
  aggregate.evaluated_at = now_epoch;
  int count = 0;
  for (const auto& item : devices_) {
    if (item.second.tenant_id != tenant_id || item.second.location_id != location_id) continue;
    auto risk = device_risk_locked(item.second, now_epoch);
    aggregate.score += risk.score;
    aggregate.factors.insert(aggregate.factors.end(), risk.factors.begin(), risk.factors.end());
    ++count;
  }
  aggregate.score = count ? aggregate.score / count : 100;
  if (!count) aggregate.factors.push_back({"unknown_location", 100, "location has no tenant devices"});
  aggregate.level = risk_level(aggregate.score);
  record_risk_change_locked(aggregate, now_epoch);
  save_locked();
  return aggregate;
}

void FleetOperationsService::record_risk_change_locked(const FleetRisk& risk, long now_epoch) {
  auto key = risk.tenant_id + ":" + risk.scope_type + ":" + risk.scope_id;
  auto previous = risk_levels_.find(key);
  if (previous != risk_levels_.end() && previous->second == risk.level) return;
  append_event_locked(risk.tenant_id, risk.scope_type == "device" ? risk.scope_id : "",
                      "risk_changed", now_epoch,
                      {{"scope_type", risk.scope_type}, {"scope_id", risk.scope_id},
                       {"from", previous == risk_levels_.end() ? "unknown" : previous->second},
                       {"to", risk.level}, {"score", risk.score}});
  risk_levels_[key] = risk.level;
}

void FleetOperationsService::quarantine_locked(FleetDevice& device,
                                               const std::string& reason,
                                               long now_epoch) {
  if (device.retired) return;
  device.lifecycle_state = "quarantined";
  device.quarantine_reason = reason;
  append_event_locked(device.tenant_id, device.device_id, "device_quarantined", now_epoch,
                      {{"reason", reason}});
  emit_evidence_locked(device, "device_quarantine", now_epoch, {{"reason", reason}});
}

bool FleetOperationsService::quarantine(const std::string& tenant_id,
                                        const std::string& device_id,
                                        const std::string& reason, long now_epoch,
                                        std::string* error) {
  std::lock_guard<std::mutex> lk(mu_);
  auto it = devices_.find(device_id);
  if (it == devices_.end() || it->second.tenant_id != tenant_id || it->second.retired) {
    if (error) *error = "not_found_or_retired";
    return false;
  }
  quarantine_locked(it->second, reason.empty() ? "operator_quarantine" : reason, now_epoch);
  save_locked();
  return true;
}

bool FleetOperationsService::recover(const std::string& tenant_id,
                                     const std::string& device_id, long now_epoch,
                                     std::string* error) {
  std::lock_guard<std::mutex> lk(mu_);
  auto it = devices_.find(device_id);
  if (it == devices_.end() || it->second.tenant_id != tenant_id) {
    if (error) *error = "not_found";
    return false;
  }
  auto& device = it->second;
  if (device.lifecycle_state != "quarantined" || device.retired) {
    if (error) *error = "device_not_recoverable";
    return false;
  }
  if (!heartbeat_fresh_locked(device, now_epoch)) {
    if (error) *error = "fresh_heartbeat_required";
    return false;
  }
  auto hb = heartbeats_.find(device_id);
  if (hb == heartbeats_.end() || !hb->second.health_metrics.value("self_test_passed", false)) {
    if (error) *error = "successful_self_test_required";
    return false;
  }
  if (hb->second.connectivity_status != "online" || hb->second.firmware_version.empty() ||
      hb->second.firmware_version == "unknown" || hb->second.driver_version.empty() ||
      hb->second.driver_version == "unknown" ||
      hb->second.inventory_summary.value("contradiction", false) ||
      !capability_known_locked(device)) {
    if (error) *error = "device_identity_or_health_unresolved";
    return false;
  }
  auto risk = device_risk_locked(device, now_epoch);
  int unresolved = risk.score;
  for (const auto& factor : risk.factors) {
    if (factor.factor == "quarantined") unresolved -= factor.points;
  }
  if (unresolved >= 75) {
    if (error) *error = "critical_risk_unresolved";
    return false;
  }
  if (!trust_ || !trust_->policy_allows_or_fail_closed(
                     tenant_id, device_id, "device_recovery",
                     {{"quarantine_reason", device.quarantine_reason},
                      {"risk_score", unresolved}, {"self_test_passed", true}},
                     now_epoch)) {
    if (error) *error = "veil_recovery_denied";
    return false;
  }
  auto previous_reason = device.quarantine_reason;
  device.lifecycle_state = "active";
  device.quarantine_reason.clear();
  append_event_locked(tenant_id, device_id, "device_recovered", now_epoch,
                      {{"previous_reason", previous_reason}, {"risk_score", unresolved}});
  emit_evidence_locked(device, "device_recovery", now_epoch,
                       {{"previous_reason", previous_reason}, {"risk_score", unresolved}});
  save_locked();
  return true;
}

std::vector<FleetDevice> FleetOperationsService::quarantined(const std::string& tenant_id) const {
  std::lock_guard<std::mutex> lk(mu_);
  std::vector<FleetDevice> out;
  for (const auto& item : devices_) {
    if (item.second.tenant_id == tenant_id && item.second.lifecycle_state == "quarantined")
      out.push_back(item.second);
  }
  return out;
}

void FleetOperationsService::append_event_locked(const std::string& tenant_id,
                                                 const std::string& device_id,
                                                 const std::string& type, long now_epoch,
                                                 nlohmann::json payload) {
  FleetOperationEvent event;
  event.sequence = static_cast<long>(events_.size()) + 1;
  event.event_id = id_for("fleet-event", events_.size());
  event.tenant_id = tenant_id;
  event.device_id = device_id;
  event.event_type = type;
  event.occurred_at = now_epoch;
  event.payload = std::move(payload);
  event.previous_hash = events_.empty() ? "" : events_.back().event_hash;
  event.event_hash = event_hash(event);
  events_.push_back(event);
}

std::string FleetOperationsService::event_hash(const FleetOperationEvent& event) const {
  return integrations::veil::sha256_hex(
      nlohmann::json{{"sequence", event.sequence}, {"event_id", event.event_id},
                     {"tenant_id", event.tenant_id}, {"device_id", event.device_id},
                     {"event_type", event.event_type}, {"occurred_at", event.occurred_at},
                     {"payload", event.payload}, {"previous_hash", event.previous_hash}}
          .dump());
}

std::vector<FleetOperationEvent> FleetOperationsService::events(const std::string& tenant_id) const {
  std::lock_guard<std::mutex> lk(mu_);
  std::vector<FleetOperationEvent> out;
  for (const auto& event : events_) {
    if (event.tenant_id == tenant_id) out.push_back(event);
  }
  return out;
}

ReplayResult FleetOperationsService::replay(const std::string& tenant_id) const {
  std::lock_guard<std::mutex> lk(mu_);
  ReplayResult result;
  std::string previous;
  long expected_sequence = 1;
  for (const auto& event : events_) {
    if (event.sequence != expected_sequence || event.previous_hash != previous ||
        event.event_hash != event_hash(event)) {
      result.valid = false;
      result.reason = "event_chain_invalid";
      break;
    }
    previous = event.event_hash;
    ++expected_sequence;
    if (event.tenant_id == tenant_id) result.events.push_back(event);
  }
  nlohmann::json hashes = nlohmann::json::array();
  for (const auto& event : result.events) hashes.push_back(event.event_hash);
  result.hash_summary = integrations::veil::sha256_hex(hashes.dump());
  return result;
}

void FleetOperationsService::emit_evidence_locked(const FleetDevice& device,
                                                  const std::string& event_type,
                                                  long now_epoch,
                                                  nlohmann::json payload) {
  if (!trust_) return;
  integrations::veil::TrustEvidenceRecord evidence;
  evidence.tenant_id = device.tenant_id;
  evidence.device_id = device.device_id;
  evidence.merchant_id = device.merchant_id;
  evidence.branch_id = device.branch_id;
  evidence.region = device.region;
  evidence.event_type = event_type;
  evidence.source_module = "fleet_operations";
  evidence.timestamp = now_epoch;
  evidence.policy_context = {{"lifecycle_state", device.lifecycle_state},
                             {"location_id", device.location_id}};
  evidence.classification_labels = {"fleet_operations"};
  evidence.sensitivity_labels = {"device_control"};
  evidence.audit_event_id = id_for("fleet-audit", events_.size());
  evidence.correlation_id = device.device_id + ":" + event_type + ":" + std::to_string(now_epoch);
  evidence.payload = std::move(payload);
  trust_->create_evidence(evidence);
}

void FleetOperationsService::save() const {
  std::lock_guard<std::mutex> lk(mu_);
  save_locked();
}

void FleetOperationsService::load() {
  if (store_path_.empty()) return;
  std::ifstream in(store_path_);
  if (!in) return;
  nlohmann::json j;
  in >> j;
  for (const auto& value : j.value("fleets", nlohmann::json::array())) {
    auto v = value.get<Fleet>(); fleets_[v.fleet_id] = v;
  }
  for (const auto& value : j.value("locations", nlohmann::json::array())) {
    auto v = value.get<Location>(); locations_[v.location_id] = v;
  }
  for (const auto& value : j.value("groups", nlohmann::json::array())) {
    auto v = value.get<DeviceGroup>(); groups_[v.group_id] = v;
  }
  for (const auto& value : j.value("enrollments", nlohmann::json::array())) {
    auto v = value.get<DeviceEnrollment>(); enrollments_[v.device_id] = v;
  }
  for (const auto& value : j.value("assignments", nlohmann::json::array())) {
    auto v = value.get<DeviceAssignment>(); assignments_[v.device_id] = v;
  }
  for (const auto& value : j.value("devices", nlohmann::json::array())) {
    auto v = value.get<FleetDevice>(); devices_[v.device_id] = v;
  }
  for (const auto& value : j.value("heartbeats", nlohmann::json::array())) {
    auto v = value.get<FleetHeartbeat>(); heartbeats_[v.device_id] = v;
  }
  for (const auto& value : j.value("commands", nlohmann::json::array())) {
    auto v = value.get<FleetCommand>(); commands_[v.command_id] = v;
  }
  policy_violations_ = j.value("policy_violations", policy_violations_);
  risk_levels_ = j.value("risk_levels", risk_levels_);
  events_ = j.value("events", events_);
}

void FleetOperationsService::save_locked() const {
  if (store_path_.empty()) return;
  auto parent = std::filesystem::path(store_path_).parent_path();
  if (!parent.empty()) std::filesystem::create_directories(parent);
  std::ofstream(store_path_, std::ios::trunc)
      << nlohmann::json{{"schema", 1}, {"fleets", map_values(fleets_)},
                        {"locations", map_values(locations_)}, {"groups", map_values(groups_)},
                        {"enrollments", map_values(enrollments_)},
                        {"assignments", map_values(assignments_)}, {"devices", map_values(devices_)},
                        {"heartbeats", map_values(heartbeats_)}, {"commands", map_values(commands_)},
                        {"policy_violations", policy_violations_}, {"risk_levels", risk_levels_},
                        {"events", events_}}
             .dump(2)
      << "\n";
}

FleetOperationsService& default_fleet_operations() {
  static FleetOperationsService* service = [] {
    const char* env = std::getenv("REGISTER_MVP_FLEET_OPERATIONS_STORE");
    std::string path = env && *env ? std::string(env) : "data/fleet_operations.json";
    return new FleetOperationsService(path, &edge_platform::default_edge_platform(),
                                      &integrations::veil::default_trust_service());
  }();
  return *service;
}

}  // namespace cloud::fleet_operations
