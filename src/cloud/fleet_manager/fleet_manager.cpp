#include "cloud/fleet_manager/fleet_manager.hpp"

#include <chrono>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <numeric>
#include <sstream>

#include "obs/metrics.hpp"
#include "util/log.hpp"

namespace cloud::fleet_manager {

namespace {

std::string id_for(const std::string& prefix, const std::string& drawer_id, size_t n) {
  return prefix + "-" + drawer_id + "-" + std::to_string(n);
}

}  // namespace

FleetManager::FleetManager(std::string store_path) : store_path_(std::move(store_path)) {
  load_store();
}

std::string now_iso() {
  auto tp = std::chrono::system_clock::now();
  std::time_t tt = std::chrono::system_clock::to_time_t(tp);
  std::tm tm{};
#if defined(_WIN32)
  gmtime_s(&tm, &tt);
#else
  gmtime_r(&tt, &tm);
#endif
  std::ostringstream oss;
  oss << std::put_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
  return oss.str();
}

void to_json(nlohmann::json& j, const FleetMetrics& v) {
  j = {{"total_drawers", v.total_drawers},
       {"online_drawers", v.online_drawers},
       {"unhealthy_drawers", v.unhealthy_drawers},
       {"firmware_distribution", v.firmware_distribution},
       {"average_health_score", v.average_health_score},
       {"active_alarms", v.active_alarms}};
}

void FleetManager::load_store() {
  if (store_path_.empty()) return;
  std::ifstream in(store_path_);
  if (!in) return;
  try {
    nlohmann::json doc;
    in >> doc;
    if (!doc.contains("twins") || !doc["twins"].is_array()) return;
    for (const auto& item : doc["twins"]) {
      auto twin = enrich(item.get<device_twin::DrawerTwin>());
      if (!twin.drawer_id.empty()) twins_[twin.drawer_id] = twin;
    }
  } catch (const std::exception& e) {
    LOG_ERROR("fleet_store_load_failed", {{"path", store_path_}, {"err", e.what()}});
  }
}

void FleetManager::save_store_locked() const {
  if (store_path_.empty()) return;
  try {
    auto parent = std::filesystem::path(store_path_).parent_path();
    if (!parent.empty()) std::filesystem::create_directories(parent);
    nlohmann::json twins = nlohmann::json::array();
    for (const auto& item : twins_) twins.push_back(item.second);
    std::ofstream out(store_path_, std::ios::trunc);
    out << nlohmann::json{{"schema", 1}, {"twins", twins}}.dump(2) << "\n";
  } catch (const std::exception& e) {
    LOG_ERROR("fleet_store_save_failed", {{"path", store_path_}, {"err", e.what()}});
  }
}

device_twin::DrawerTwin FleetManager::enrich(device_twin::DrawerTwin twin) const {
  std::string ts = now_iso();
  twin.health = health_.score(twin.health);
  twin.maintenance = maintenance_.evaluate(twin.health, ts);
  twin.inventory_forecast = inventory_.forecast(twin.inventory);
  twin.alerts = alerts_.evaluate(twin, ts);
  return twin;
}

void FleetManager::upsert(device_twin::DrawerTwin twin) {
  if (twin.drawer_id.empty()) {
    return;
  }
  auto enriched = enrich(std::move(twin));
  std::lock_guard<std::mutex> lk(mu_);
  twins_[enriched.drawer_id] = enriched;
  save_store_locked();
  publish_metrics_locked(metrics_locked());
}

std::vector<device_twin::DrawerTwin> FleetManager::list() const {
  std::lock_guard<std::mutex> lk(mu_);
  std::vector<device_twin::DrawerTwin> out;
  out.reserve(twins_.size());
  for (const auto& item : twins_) {
    out.push_back(item.second);
  }
  return out;
}

std::optional<device_twin::DrawerTwin> FleetManager::get(const std::string& drawer_id) const {
  std::lock_guard<std::mutex> lk(mu_);
  auto it = twins_.find(drawer_id);
  if (it == twins_.end()) return std::nullopt;
  return it->second;
}

FleetMetrics FleetManager::metrics() const {
  std::lock_guard<std::mutex> lk(mu_);
  auto out = metrics_locked();
  publish_metrics_locked(out);
  return out;
}

FleetMetrics FleetManager::metrics_locked() const {
  FleetMetrics out;
  out.total_drawers = static_cast<int>(twins_.size());
  int health_sum = 0;
  for (const auto& item : twins_) {
    const auto& twin = item.second;
    if (twin.connectivity_status == "online") ++out.online_drawers;
    if (twin.health.score < 70) ++out.unhealthy_drawers;
    ++out.firmware_distribution[twin.firmware.current_version];
    health_sum += twin.health.score;
    for (const auto& alert : twin.alerts) {
      if (alert.severity == "warning" || alert.severity == "critical") ++out.active_alarms;
    }
  }
  out.average_health_score =
      out.total_drawers == 0 ? 0.0 : static_cast<double>(health_sum) / out.total_drawers;
  return out;
}

void FleetManager::publish_metrics_locked(const FleetMetrics& metrics) const {
  obs::M().gauge("register_fleet_total_drawers", "Fleet drawer count").set(metrics.total_drawers);
  obs::M().gauge("register_fleet_online_drawers", "Online fleet drawer count").set(metrics.online_drawers);
  obs::M().gauge("register_fleet_unhealthy_drawers", "Unhealthy fleet drawer count")
      .set(metrics.unhealthy_drawers);
  obs::M().gauge("register_fleet_average_health_score", "Average fleet health score")
      .set(metrics.average_health_score);
  obs::M().gauge("register_fleet_active_alarms", "Active fleet alarm count").set(metrics.active_alarms);

  std::map<std::pair<std::string, std::string>, int> alert_counts;
  for (const auto& item : twins_) {
    for (const auto& alert : item.second.alerts) {
      ++alert_counts[{alert.severity, alert.type}];
    }
  }
  for (const auto& item : alert_counts) {
    obs::M()
        .gauge("register_fleet_alerts", "Fleet alerts by severity and type",
               {{"severity", item.first.first}, {"type", item.first.second}})
        .set(item.second);
  }
}

void FleetManager::record_history(const std::string& drawer_id, device_twin::HistoryEvent event) {
  std::lock_guard<std::mutex> lk(mu_);
  auto it = twins_.find(drawer_id);
  if (it == twins_.end()) return;
  if (event.id.empty()) event.id = id_for("hist", drawer_id, it->second.history.size() + 1);
  if (event.occurred_at.empty()) event.occurred_at = now_iso();
  it->second.history.push_back(std::move(event));
  save_store_locked();
  publish_metrics_locked(metrics_locked());
}

device_twin::DrawerTwin make_local_default_twin() {
  auto ts = now_iso();
  device_twin::DrawerTwin twin;
  twin.drawer_id = "local-drawer";
  twin.merchant_id = "local-merchant";
  twin.firmware.current_version = "1.0.0";
  twin.firmware.target_version = "1.0.0";
  twin.firmware.hardware_revision = "revA";
  twin.connectivity_status = "online";
  twin.first_seen_at = ts;
  twin.last_telemetry_at = ts;
  twin.health.motor_cycles = 2500;
  twin.health.transaction_latency_ms = 450.0;
  twin.health.uptime_percent = 99.9;
  twin.inventory.denominations["quarter"] = {"quarter", 220, 400, 12.0, ts};
  twin.inventory.denominations["dime"] = {"dime", 180, 300, 4.0, ts};
  twin.inventory.denominations["nickel"] = {"nickel", 150, 250, 2.0, ts};
  twin.inventory.denominations["penny"] = {"penny", 300, 500, 6.0, ts};
  twin.history.push_back({"hist-local-drawer-1", "telemetry", "device twin initialized", ts,
                          {{"source", "local_runtime"}}});
  return twin;
}

FleetManager& default_manager() {
  static FleetManager* manager = [] {
    const char* env = std::getenv("REGISTER_MVP_TWIN_STORE");
    std::string path = env && *env ? std::string(env) : "data/device_twin.json";
    auto* m = new FleetManager(path);
    if (m->list().empty()) m->upsert(make_local_default_twin());
    return m;
  }();
  return *manager;
}

}  // namespace cloud::fleet_manager
