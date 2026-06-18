#pragma once

#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

#include "cloud/fleet_control/control_plane.hpp"
#include "edge_platform/driver.hpp"
#include "integrations/veil/service.hpp"

namespace edge_platform {

class EdgePlatformService {
 public:
  explicit EdgePlatformService(std::string store_path = "",
                               cloud::fleet_control::FleetControlPlane* fleet = nullptr,
                               integrations::veil::VeilTrustService* trust = nullptr);

  bool register_device(EdgeDevice device);
  bool register_capability(const std::string& device_id, CapabilityMetadata capability);
  std::vector<EdgeDevice> devices(const std::string& tenant_id = "") const;
  std::vector<CapabilityMetadata> capabilities(const std::string& device_id) const;
  std::vector<DriverMetadata> drivers() const;

  EdgeCommandResult execute_command(EdgeCommandRequest request);
  EdgeInventoryView inventory(const std::string& device_id) const;
  bool update_inventory(EdgeInventoryView inventory);
  SimulatorState simulate(const std::string& device_id, const std::string& action,
                          const nlohmann::json& parameters, long now_epoch);
  EdgeHealthSummary health(const std::string& device_id);
  std::vector<EdgeEvent> events(const std::string& tenant_id = "") const;

  void save() const;
  void load();

 private:
  bool known_device_type(const std::string& device_type) const;
  std::string capability_for_command(const std::string& command) const;
  bool high_risk_command(const std::string& command) const;
  bool has_capability_locked(const EdgeDevice& device, const std::string& capability) const;
  EdgeInventoryView default_inventory_for(const EdgeDevice& device) const;
  EdgeHealthSummary health_locked(const std::string& device_id);
  void register_default_driver_locked(const std::string& device_type);
  void emit_event_locked(const std::string& tenant_id, const std::string& device_id,
                         const std::string& event_type, long now_epoch, nlohmann::json metadata = {});
  void emit_trust_evidence_locked(const EdgeDevice& device, const std::string& event_type,
                                  long now_epoch, nlohmann::json payload);
  void save_locked() const;

  mutable std::mutex mu_;
  std::string store_path_;
  cloud::fleet_control::FleetControlPlane* fleet_{nullptr};
  integrations::veil::VeilTrustService* trust_{nullptr};
  std::unordered_map<std::string, EdgeDevice> devices_;
  std::unordered_map<std::string, std::vector<CapabilityMetadata>> capabilities_;
  std::unordered_map<std::string, std::unique_ptr<EdgeDriver>> drivers_;
  std::unordered_map<std::string, EdgeInventoryView> inventories_;
  std::unordered_map<std::string, SimulatorState> simulator_;
  std::unordered_map<std::string, int> command_count_;
  std::unordered_map<std::string, int> command_failures_;
  std::vector<EdgeCommandResult> command_results_;
  std::vector<EdgeEvent> events_;
};

EdgePlatformService& default_edge_platform();

}  // namespace edge_platform
