#pragma once

#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "cloud/fleet_operations/models.hpp"
#include "edge_platform/service.hpp"
#include "integrations/veil/service.hpp"

namespace cloud::fleet_operations {

class FleetOperationsService {
 public:
  explicit FleetOperationsService(std::string store_path = "",
                                  edge_platform::EdgePlatformService* edge = nullptr,
                                  integrations::veil::VeilTrustService* trust = nullptr,
                                  long heartbeat_timeout_seconds = 90);

  bool enroll(FleetDevice device, const std::string& actor_id, long now_epoch,
              std::string* error = nullptr);
  std::optional<FleetDevice> device(const std::string& tenant_id,
                                    const std::string& device_id) const;
  std::vector<FleetDevice> devices(const std::string& tenant_id) const;
  bool update_device(const std::string& tenant_id, const std::string& device_id,
                     const nlohmann::json& patch, long now_epoch, std::string* error = nullptr);
  bool retire_device(const std::string& tenant_id, const std::string& device_id,
                     long now_epoch, std::string* error = nullptr);

  bool receive_heartbeat(FleetHeartbeat heartbeat, long received_at,
                         std::string* error = nullptr);
  nlohmann::json health(const std::string& tenant_id, const std::string& device_id,
                        long now_epoch) const;
  nlohmann::json freshness(const std::string& tenant_id, const std::string& device_id,
                           long now_epoch) const;

  FleetCommand create_command(FleetCommand command, long now_epoch);
  std::optional<FleetCommand> command(const std::string& tenant_id,
                                      const std::string& command_id) const;
  std::vector<FleetCommand> commands(const std::string& tenant_id) const;
  bool cancel_command(const std::string& tenant_id, const std::string& command_id,
                      long now_epoch, std::string* error = nullptr);
  bool acknowledge_command(const std::string& tenant_id, const std::string& command_id,
                           long now_epoch, std::string* error = nullptr);
  bool complete_command(const std::string& tenant_id, const std::string& command_id,
                        bool success, const std::string& result, long now_epoch,
                        std::string* error = nullptr);
  int expire_commands(long now_epoch);

  FleetRisk device_risk(const std::string& tenant_id, const std::string& device_id,
                        long now_epoch);
  std::vector<FleetRisk> fleet_risk(const std::string& tenant_id, long now_epoch);
  FleetRisk location_risk(const std::string& tenant_id, const std::string& location_id,
                          long now_epoch);

  bool quarantine(const std::string& tenant_id, const std::string& device_id,
                  const std::string& reason, long now_epoch, std::string* error = nullptr);
  bool recover(const std::string& tenant_id, const std::string& device_id,
               long now_epoch, std::string* error = nullptr);
  std::vector<FleetDevice> quarantined(const std::string& tenant_id) const;

  std::vector<FleetOperationEvent> events(const std::string& tenant_id) const;
  ReplayResult replay(const std::string& tenant_id) const;
  void save() const;
  void load();

 private:
  bool valid_transition(const std::string& from, const std::string& to,
                        bool recovery = false) const;
  std::string capability_for_command(const std::string& command) const;
  bool high_risk_command(const std::string& command) const;
  bool heartbeat_fresh_locked(const FleetDevice& device, long now_epoch) const;
  bool capability_known_locked(const FleetDevice& device) const;
  FleetRisk device_risk_locked(const FleetDevice& device, long now_epoch) const;
  void quarantine_locked(FleetDevice& device, const std::string& reason, long now_epoch);
  void record_risk_change_locked(const FleetRisk& risk, long now_epoch);
  void append_event_locked(const std::string& tenant_id, const std::string& device_id,
                           const std::string& type, long now_epoch,
                           nlohmann::json payload = nlohmann::json::object());
  std::string event_hash(const FleetOperationEvent& event) const;
  void emit_evidence_locked(const FleetDevice& device, const std::string& event_type,
                            long now_epoch, nlohmann::json payload);
  void save_locked() const;

  mutable std::mutex mu_;
  std::string store_path_;
  edge_platform::EdgePlatformService* edge_{nullptr};
  integrations::veil::VeilTrustService* trust_{nullptr};
  long heartbeat_timeout_seconds_{90};
  std::unordered_map<std::string, Fleet> fleets_;
  std::unordered_map<std::string, Location> locations_;
  std::unordered_map<std::string, DeviceGroup> groups_;
  std::unordered_map<std::string, DeviceEnrollment> enrollments_;
  std::unordered_map<std::string, DeviceAssignment> assignments_;
  std::unordered_map<std::string, FleetDevice> devices_;
  std::unordered_map<std::string, FleetHeartbeat> heartbeats_;
  std::unordered_map<std::string, FleetCommand> commands_;
  std::unordered_map<std::string, int> policy_violations_;
  std::unordered_map<std::string, std::string> risk_levels_;
  std::vector<FleetOperationEvent> events_;
};

FleetOperationsService& default_fleet_operations();

}  // namespace cloud::fleet_operations
