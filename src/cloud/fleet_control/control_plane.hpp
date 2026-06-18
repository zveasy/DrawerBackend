#pragma once

#include <mutex>
#include <optional>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

#include "cloud/fleet_control/models.hpp"

namespace cloud::fleet_control {

class FleetControlPlane {
 public:
  explicit FleetControlPlane(std::string store_path = "", long heartbeat_timeout_seconds = 90);

  void upsert_organization(Organization org);
  void upsert_region(Region region);
  void upsert_merchant(Merchant merchant);
  void upsert_branch(Branch branch);

  std::vector<Organization> organizations(const std::string& tenant_org = "") const;
  std::vector<Merchant> merchants(const std::string& tenant_org = "") const;
  std::vector<Branch> branches(const std::string& tenant_org = "") const;
  std::vector<Region> regions(const std::string& tenant_org = "") const;

  bool register_device(DeviceRegistryRecord device);
  std::optional<DeviceRegistryRecord> device(const std::string& device_id) const;
  std::vector<DeviceRegistryRecord> search_devices(const DeviceFilter& filter) const;

  bool receive_heartbeat(const HeartbeatMessage& heartbeat);
  int detect_stale_devices(long now_epoch);

  RemoteCommand create_command(const std::string& organization_id, const std::string& device_id,
                               const std::string& type, long now_epoch, long ttl_seconds,
                               int max_retries = 3);
  std::optional<RemoteCommand> deliver_next_command(const std::string& device_id, long now_epoch);
  bool acknowledge_command(const std::string& device_id, const std::string& command_id, long now_epoch);
  int expire_commands(long now_epoch);
  std::vector<RemoteCommand> commands(const std::string& organization_id = "",
                                      const std::string& device_id = "") const;

  FleetAlert create_alert(const std::string& organization_id, const std::string& device_id,
                          const std::string& type, const std::string& severity,
                          const std::string& message, long now_epoch);
  bool acknowledge_alert(const std::string& organization_id, const std::string& alert_id,
                         long now_epoch);
  std::vector<FleetAlert> alerts(const std::string& organization_id = "",
                                 bool include_acknowledged = false) const;

  FleetHealthSummary health(const std::string& organization_id = "") const;
  FleetMetrics metrics(const std::string& organization_id = "") const;
  DashboardView dashboard(const std::string& scope_type, const std::string& scope_id) const;
  std::vector<FleetEvent> events(const std::string& organization_id = "") const;

  void save() const;
  void load();

 private:
  void save_locked() const;
  void event_locked(const std::string& organization_id, const std::string& device_id,
                    const std::string& type, long now_epoch, nlohmann::json metadata = {});
  bool tenant_matches(const DeviceRegistryRecord& d, const DeviceFilter& filter) const;
  bool known_command_type(const std::string& type) const;
  void publish_metrics_locked(const FleetMetrics& metrics) const;

  mutable std::mutex mu_;
  std::string store_path_;
  long heartbeat_timeout_seconds_;
  std::unordered_map<std::string, Organization> organizations_;
  std::unordered_map<std::string, Region> regions_;
  std::unordered_map<std::string, Merchant> merchants_;
  std::unordered_map<std::string, Branch> branches_;
  std::unordered_map<std::string, DeviceRegistryRecord> devices_;
  std::unordered_map<std::string, RemoteCommand> commands_;
  std::set<std::string> acknowledged_commands_;
  std::vector<FleetAlert> alerts_;
  std::vector<FleetEvent> events_;
  double heartbeat_latency_sum_{0.0};
  int heartbeat_count_{0};
};

std::string epoch_string(long epoch);

}  // namespace cloud::fleet_control
