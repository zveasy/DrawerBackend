#include "cloud/fleet_control/control_plane.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>

#include "obs/metrics.hpp"

namespace cloud::fleet_control {
namespace {

bool has_tag(const std::vector<std::string>& tags, const std::string& tag) {
  return tag.empty() || std::find(tags.begin(), tags.end(), tag) != tags.end();
}

bool label_matches(const Labels& labels, const std::string& key, const std::string& value) {
  if (key.empty()) return true;
  auto it = labels.find(key);
  if (it == labels.end()) return false;
  return value.empty() || it->second == value;
}

bool contains_text(const DeviceRegistryRecord& d, const std::string& text) {
  if (text.empty()) return true;
  return d.device_id.find(text) != std::string::npos || d.merchant_id.find(text) != std::string::npos ||
         d.branch_id.find(text) != std::string::npos || d.region.find(text) != std::string::npos;
}

std::string id_for(const std::string& prefix, size_t n) {
  return prefix + "-" + std::to_string(n + 1);
}

}  // namespace

std::string epoch_string(long epoch) {
  return std::to_string(epoch);
}

FleetControlPlane::FleetControlPlane(std::string store_path, long heartbeat_timeout_seconds)
    : store_path_(std::move(store_path)), heartbeat_timeout_seconds_(heartbeat_timeout_seconds) {
  load();
}

void FleetControlPlane::upsert_organization(Organization org) {
  std::lock_guard<std::mutex> lk(mu_);
  organizations_[org.organization_id] = std::move(org);
  save_locked();
}

void FleetControlPlane::upsert_region(Region region) {
  std::lock_guard<std::mutex> lk(mu_);
  regions_[region.region_id] = std::move(region);
  save_locked();
}

void FleetControlPlane::upsert_merchant(Merchant merchant) {
  std::lock_guard<std::mutex> lk(mu_);
  merchants_[merchant.merchant_id] = std::move(merchant);
  save_locked();
}

void FleetControlPlane::upsert_branch(Branch branch) {
  std::lock_guard<std::mutex> lk(mu_);
  branches_[branch.branch_id] = std::move(branch);
  save_locked();
}

std::vector<Organization> FleetControlPlane::organizations(const std::string& tenant_org) const {
  std::lock_guard<std::mutex> lk(mu_);
  std::vector<Organization> out;
  for (const auto& item : organizations_) {
    if (tenant_org.empty() || item.second.organization_id == tenant_org) out.push_back(item.second);
  }
  return out;
}

std::vector<Merchant> FleetControlPlane::merchants(const std::string& tenant_org) const {
  std::lock_guard<std::mutex> lk(mu_);
  std::vector<Merchant> out;
  for (const auto& item : merchants_) {
    if (tenant_org.empty() || item.second.organization_id == tenant_org) out.push_back(item.second);
  }
  return out;
}

std::vector<Branch> FleetControlPlane::branches(const std::string& tenant_org) const {
  std::lock_guard<std::mutex> lk(mu_);
  std::vector<Branch> out;
  for (const auto& item : branches_) {
    if (tenant_org.empty() || item.second.organization_id == tenant_org) out.push_back(item.second);
  }
  return out;
}

std::vector<Region> FleetControlPlane::regions(const std::string& tenant_org) const {
  std::lock_guard<std::mutex> lk(mu_);
  std::vector<Region> out;
  for (const auto& item : regions_) {
    if (tenant_org.empty() || item.second.organization_id == tenant_org) out.push_back(item.second);
  }
  return out;
}

bool FleetControlPlane::register_device(DeviceRegistryRecord device) {
  if (device.device_id.empty() || device.organization_id.empty()) return false;
  std::lock_guard<std::mutex> lk(mu_);
  auto org_id = device.organization_id;
  auto device_id = device.device_id;
  devices_[device.device_id] = std::move(device);
  event_locked(org_id, device_id, "device_registered", 0);
  save_locked();
  return true;
}

std::optional<DeviceRegistryRecord> FleetControlPlane::device(const std::string& device_id) const {
  std::lock_guard<std::mutex> lk(mu_);
  auto it = devices_.find(device_id);
  if (it == devices_.end()) return std::nullopt;
  return it->second;
}

bool FleetControlPlane::tenant_matches(const DeviceRegistryRecord& d, const DeviceFilter& filter) const {
  if (!filter.organization_id.empty() && d.organization_id != filter.organization_id) return false;
  if (!filter.merchant_id.empty() && d.merchant_id != filter.merchant_id) return false;
  if (!filter.branch_id.empty() && d.branch_id != filter.branch_id) return false;
  if (!filter.region.empty() && d.region != filter.region) return false;
  if (!filter.environment.empty() && d.environment != filter.environment) return false;
  if (!filter.deployment_channel.empty() && d.deployment_channel != filter.deployment_channel) return false;
  if (!filter.health_status.empty() && d.health_status != filter.health_status) return false;
  if (!filter.connectivity_status.empty() && d.connectivity_status != filter.connectivity_status) return false;
  if (!has_tag(d.tags, filter.tag)) return false;
  if (!label_matches(d.labels, filter.label_key, filter.label_value)) return false;
  return contains_text(d, filter.text);
}

std::vector<DeviceRegistryRecord> FleetControlPlane::search_devices(const DeviceFilter& filter) const {
  std::lock_guard<std::mutex> lk(mu_);
  std::vector<DeviceRegistryRecord> out;
  for (const auto& item : devices_) {
    if (tenant_matches(item.second, filter)) out.push_back(item.second);
  }
  return out;
}

bool FleetControlPlane::receive_heartbeat(const HeartbeatMessage& heartbeat) {
  std::lock_guard<std::mutex> lk(mu_);
  auto it = devices_.find(heartbeat.device_id);
  if (it == devices_.end()) return false;
  auto& d = it->second;
  bool was_online = d.connectivity_status == "online";
  d.connectivity_status = "online";
  d.last_heartbeat_at = epoch_string(heartbeat.received_at_epoch);
  d.last_seen_at = d.last_heartbeat_at;
  heartbeat_latency_sum_ += heartbeat.latency_ms;
  ++heartbeat_count_;
  event_locked(d.organization_id, d.device_id, "heartbeat_received", heartbeat.received_at_epoch,
               {{"latency_ms", heartbeat.latency_ms}});
  if (!was_online) event_locked(d.organization_id, d.device_id, "device_online", heartbeat.received_at_epoch);
  save_locked();
  return true;
}

int FleetControlPlane::detect_stale_devices(long now_epoch) {
  std::lock_guard<std::mutex> lk(mu_);
  int stale = 0;
  for (auto& item : devices_) {
    auto& d = item.second;
    long last = d.last_seen_at.empty() ? 0 : std::stol(d.last_seen_at);
    if (d.connectivity_status == "online" && now_epoch - last > heartbeat_timeout_seconds_) {
      d.connectivity_status = "offline";
      ++stale;
      event_locked(d.organization_id, d.device_id, "device_offline", now_epoch);
      alerts_.push_back({id_for("alert", alerts_.size()), d.organization_id, d.device_id,
                         "offline_device", "critical", "device heartbeat is stale",
                         epoch_string(now_epoch), false, ""});
      event_locked(d.organization_id, d.device_id, "alert_created", now_epoch,
                   {{"type", "offline_device"}});
    }
  }
  save_locked();
  return stale;
}

bool FleetControlPlane::known_command_type(const std::string& type) const {
  static const std::set<std::string> allowed = {"restart", "refresh_config", "sync_inventory",
                                               "pause_transactions", "resume_transactions",
                                               "disable_device", "enable_device"};
  return allowed.count(type) > 0;
}

RemoteCommand FleetControlPlane::create_command(const std::string& organization_id,
                                                const std::string& device_id,
                                                const std::string& type, long now_epoch,
                                                long ttl_seconds, int max_retries) {
  std::lock_guard<std::mutex> lk(mu_);
  RemoteCommand cmd;
  cmd.command_id = id_for("cmd", commands_.size());
  cmd.organization_id = organization_id;
  cmd.device_id = device_id;
  cmd.type = type;
  cmd.created_at = epoch_string(now_epoch);
  cmd.expires_at_epoch = now_epoch + ttl_seconds;
  cmd.max_retries = max_retries;
  if (!known_command_type(type) || devices_.find(device_id) == devices_.end()) {
    cmd.status = "failed";
    cmd.failure_reason = "invalid_command";
  }
  commands_[cmd.command_id] = cmd;
  event_locked(organization_id, device_id, "command_created", now_epoch, {{"command_id", cmd.command_id}});
  save_locked();
  return cmd;
}

std::optional<RemoteCommand> FleetControlPlane::deliver_next_command(const std::string& device_id,
                                                                     long now_epoch) {
  std::lock_guard<std::mutex> lk(mu_);
  for (auto& item : commands_) {
    auto& cmd = item.second;
    if (cmd.device_id != device_id || (cmd.status != "pending" && cmd.status != "delivered")) continue;
    if (acknowledged_commands_.count(cmd.command_id)) continue;
    if (now_epoch > cmd.expires_at_epoch) {
      cmd.status = "expired";
      cmd.failed_at = epoch_string(now_epoch);
      cmd.failure_reason = "expired";
      event_locked(cmd.organization_id, device_id, "command_failed", now_epoch,
                   {{"command_id", cmd.command_id}, {"reason", "expired"}});
      continue;
    }
    if (cmd.status == "delivered") {
      if (cmd.retry_count >= cmd.max_retries) {
        cmd.status = "failed";
        cmd.failed_at = epoch_string(now_epoch);
        cmd.failure_reason = "max_retries";
        event_locked(cmd.organization_id, device_id, "command_failed", now_epoch,
                     {{"command_id", cmd.command_id}, {"reason", "max_retries"}});
        continue;
      }
      ++cmd.retry_count;
    }
    cmd.status = "delivered";
    cmd.delivered_at = epoch_string(now_epoch);
    event_locked(cmd.organization_id, device_id, "command_delivered", now_epoch,
                 {{"command_id", cmd.command_id}, {"retry_count", cmd.retry_count}});
    save_locked();
    return cmd;
  }
  save_locked();
  return std::nullopt;
}

bool FleetControlPlane::acknowledge_command(const std::string& device_id, const std::string& command_id,
                                            long now_epoch) {
  std::lock_guard<std::mutex> lk(mu_);
  auto it = commands_.find(command_id);
  if (it == commands_.end() || it->second.device_id != device_id) return false;
  if (acknowledged_commands_.count(command_id)) return false;
  it->second.status = "acknowledged";
  it->second.acknowledged_at = epoch_string(now_epoch);
  acknowledged_commands_.insert(command_id);
  event_locked(it->second.organization_id, device_id, "command_acknowledged", now_epoch,
               {{"command_id", command_id}});
  save_locked();
  return true;
}

int FleetControlPlane::expire_commands(long now_epoch) {
  std::lock_guard<std::mutex> lk(mu_);
  int expired = 0;
  for (auto& item : commands_) {
    auto& cmd = item.second;
    if ((cmd.status == "pending" || cmd.status == "delivered") && now_epoch > cmd.expires_at_epoch) {
      cmd.status = "expired";
      cmd.failed_at = epoch_string(now_epoch);
      cmd.failure_reason = "expired";
      ++expired;
      event_locked(cmd.organization_id, cmd.device_id, "command_failed", now_epoch,
                   {{"command_id", cmd.command_id}, {"reason", "expired"}});
    }
  }
  save_locked();
  return expired;
}

std::vector<RemoteCommand> FleetControlPlane::commands(const std::string& organization_id,
                                                       const std::string& device_id) const {
  std::lock_guard<std::mutex> lk(mu_);
  std::vector<RemoteCommand> out;
  for (const auto& item : commands_) {
    const auto& c = item.second;
    if (!organization_id.empty() && c.organization_id != organization_id) continue;
    if (!device_id.empty() && c.device_id != device_id) continue;
    out.push_back(c);
  }
  return out;
}

FleetAlert FleetControlPlane::create_alert(const std::string& organization_id, const std::string& device_id,
                                           const std::string& type, const std::string& severity,
                                           const std::string& message, long now_epoch) {
  std::lock_guard<std::mutex> lk(mu_);
  FleetAlert alert{id_for("alert", alerts_.size()), organization_id, device_id, type, severity,
                   message, epoch_string(now_epoch), false, ""};
  alerts_.push_back(alert);
  event_locked(organization_id, device_id, "alert_created", now_epoch, {{"type", type}});
  save_locked();
  return alert;
}

bool FleetControlPlane::acknowledge_alert(const std::string& organization_id, const std::string& alert_id,
                                          long now_epoch) {
  std::lock_guard<std::mutex> lk(mu_);
  for (auto& alert : alerts_) {
    if (alert.alert_id == alert_id && alert.organization_id == organization_id && !alert.acknowledged) {
      alert.acknowledged = true;
      alert.acknowledged_at = epoch_string(now_epoch);
      event_locked(organization_id, alert.device_id, "alert_acknowledged", now_epoch,
                   {{"alert_id", alert_id}});
      save_locked();
      return true;
    }
  }
  return false;
}

std::vector<FleetAlert> FleetControlPlane::alerts(const std::string& organization_id,
                                                  bool include_acknowledged) const {
  std::lock_guard<std::mutex> lk(mu_);
  std::vector<FleetAlert> out;
  for (const auto& alert : alerts_) {
    if (!organization_id.empty() && alert.organization_id != organization_id) continue;
    if (!include_acknowledged && alert.acknowledged) continue;
    out.push_back(alert);
  }
  return out;
}

FleetHealthSummary FleetControlPlane::health(const std::string& organization_id) const {
  std::lock_guard<std::mutex> lk(mu_);
  FleetHealthSummary h;
  int count = 0, device_sum = 0, comm_sum = 0, sync_sum = 0, ota_sum = 0, cert_sum = 0;
  for (const auto& item : devices_) {
    const auto& d = item.second;
    if (!organization_id.empty() && d.organization_id != organization_id) continue;
    ++count;
    device_sum += d.health_score;
    comm_sum += d.connectivity_status == "online" ? 100 : 0;
    sync_sum += std::max(0, 100 - d.sync_failures * 20);
    ota_sum += std::max(0, 100 - d.ota_failures * 25);
    cert_sum += d.certificate_identity_status == "active" ? 100
                : d.certificate_identity_status == "pending" ? 80
                : 0;
  }
  if (count > 0) {
    h.device_health_score = device_sum / count;
    h.communication_health_score = comm_sum / count;
    h.sync_health_score = sync_sum / count;
    h.ota_health_score = ota_sum / count;
    h.certificate_health_score = cert_sum / count;
    h.aggregate_score = (h.device_health_score + h.communication_health_score + h.sync_health_score +
                         h.ota_health_score + h.certificate_health_score) /
                        5;
  }
  return h;
}

FleetMetrics FleetControlPlane::metrics(const std::string& organization_id) const {
  std::lock_guard<std::mutex> lk(mu_);
  FleetMetrics m;
  int total = 0, command_done = 0, command_failed = 0, command_total = 0;
  for (const auto& item : devices_) {
    const auto& d = item.second;
    if (!organization_id.empty() && d.organization_id != organization_id) continue;
    ++total;
    if (d.connectivity_status == "online") ++m.online_devices;
    if (d.connectivity_status == "offline") ++m.offline_devices;
    m.sync_failure_count += d.sync_failures;
    m.sync_success_count += d.sync_failures == 0 ? 1 : 0;
  }
  for (const auto& item : commands_) {
    const auto& c = item.second;
    if (!organization_id.empty() && c.organization_id != organization_id) continue;
    ++command_total;
    if (c.status == "acknowledged") ++command_done;
    if (c.status == "failed" || c.status == "expired") ++command_failed;
  }
  for (const auto& alert : alerts_) {
    if (!organization_id.empty() && alert.organization_id != organization_id) continue;
    if (!alert.acknowledged) ++m.alert_counts[alert.severity];
  }
  m.command_success_rate = command_total ? static_cast<double>(command_done) / command_total : 0.0;
  m.command_failure_rate = command_total ? static_cast<double>(command_failed) / command_total : 0.0;
  m.average_heartbeat_latency_ms = heartbeat_count_ ? heartbeat_latency_sum_ / heartbeat_count_ : 0.0;
  m.fleet_availability = total ? static_cast<double>(m.online_devices) / total : 0.0;
  publish_metrics_locked(m);
  return m;
}

DashboardView FleetControlPlane::dashboard(const std::string& scope_type, const std::string& scope_id) const {
  DeviceFilter filter;
  if (scope_type == "organization") filter.organization_id = scope_id;
  if (scope_type == "merchant") filter.merchant_id = scope_id;
  if (scope_type == "branch") filter.branch_id = scope_id;
  if (scope_type == "region") filter.region = scope_id;
  auto devices = search_devices(filter);
  DashboardView view;
  view.scope_type = scope_type;
  view.scope_id = scope_id;
  view.device_count = static_cast<int>(devices.size());
  for (const auto& d : devices) {
    if (d.connectivity_status == "online") ++view.online_devices;
    if (d.connectivity_status == "offline") ++view.offline_devices;
    ++view.firmware_distribution[d.firmware_version];
  }
  view.health = health(scope_type == "organization" ? scope_id : "");
  for (const auto& alert : alerts(scope_type == "organization" ? scope_id : "", false)) {
    ++view.alert_summary[alert.severity];
  }
  return view;
}

std::vector<FleetEvent> FleetControlPlane::events(const std::string& organization_id) const {
  std::lock_guard<std::mutex> lk(mu_);
  std::vector<FleetEvent> out;
  for (const auto& event : events_) {
    if (organization_id.empty() || event.organization_id == organization_id) out.push_back(event);
  }
  return out;
}

void FleetControlPlane::save() const {
  std::lock_guard<std::mutex> lk(mu_);
  save_locked();
}

void FleetControlPlane::load() {
  if (store_path_.empty()) return;
  std::ifstream in(store_path_);
  if (!in) return;
  nlohmann::json j;
  in >> j;
  for (const auto& item : j.value("organizations", nlohmann::json::array())) {
    auto v = item.get<Organization>();
    organizations_[v.organization_id] = v;
  }
  for (const auto& item : j.value("regions", nlohmann::json::array())) {
    auto v = item.get<Region>();
    regions_[v.region_id] = v;
  }
  for (const auto& item : j.value("merchants", nlohmann::json::array())) {
    auto v = item.get<Merchant>();
    merchants_[v.merchant_id] = v;
  }
  for (const auto& item : j.value("branches", nlohmann::json::array())) {
    auto v = item.get<Branch>();
    branches_[v.branch_id] = v;
  }
  for (const auto& item : j.value("devices", nlohmann::json::array())) {
    auto v = item.get<DeviceRegistryRecord>();
    devices_[v.device_id] = v;
  }
  for (const auto& item : j.value("commands", nlohmann::json::array())) {
    auto v = item.get<RemoteCommand>();
    commands_[v.command_id] = v;
    if (v.status == "acknowledged") acknowledged_commands_.insert(v.command_id);
  }
  alerts_ = j.value("alerts", alerts_);
  events_ = j.value("events", events_);
  heartbeat_latency_sum_ = j.value("heartbeat_latency_sum", heartbeat_latency_sum_);
  heartbeat_count_ = j.value("heartbeat_count", heartbeat_count_);
}

void FleetControlPlane::save_locked() const {
  if (store_path_.empty()) return;
  auto parent = std::filesystem::path(store_path_).parent_path();
  if (!parent.empty()) std::filesystem::create_directories(parent);
  auto values = [](const auto& map) {
    nlohmann::json out = nlohmann::json::array();
    for (const auto& item : map) out.push_back(item.second);
    return out;
  };
  std::ofstream(store_path_, std::ios::trunc)
      << nlohmann::json{{"schema", 1},
                        {"organizations", values(organizations_)},
                        {"regions", values(regions_)},
                        {"merchants", values(merchants_)},
                        {"branches", values(branches_)},
                        {"devices", values(devices_)},
                        {"commands", values(commands_)},
                        {"alerts", alerts_},
                        {"events", events_},
                        {"heartbeat_latency_sum", heartbeat_latency_sum_},
                        {"heartbeat_count", heartbeat_count_}}
             .dump(2)
      << "\n";
}

void FleetControlPlane::event_locked(const std::string& organization_id, const std::string& device_id,
                                     const std::string& type, long now_epoch, nlohmann::json metadata) {
  events_.push_back({id_for("evt", events_.size()), organization_id, device_id, type,
                     epoch_string(now_epoch), std::move(metadata)});
}

void FleetControlPlane::publish_metrics_locked(const FleetMetrics& m) const {
  obs::M().gauge("register_fleet_control_online_devices", "Online fleet-control devices").set(m.online_devices);
  obs::M().gauge("register_fleet_control_offline_devices", "Offline fleet-control devices").set(m.offline_devices);
  obs::M().gauge("register_fleet_control_command_success_rate", "Fleet command success rate")
      .set(m.command_success_rate);
  obs::M().gauge("register_fleet_control_command_failure_rate", "Fleet command failure rate")
      .set(m.command_failure_rate);
  obs::M().gauge("register_fleet_control_heartbeat_latency_ms", "Average heartbeat latency")
      .set(m.average_heartbeat_latency_ms);
  obs::M().gauge("register_fleet_control_availability", "Fleet availability").set(m.fleet_availability);
  for (const auto& item : m.alert_counts) {
    obs::M().gauge("register_fleet_control_alerts", "Fleet alert counts", {{"severity", item.first}})
        .set(item.second);
  }
}

}  // namespace cloud::fleet_control
