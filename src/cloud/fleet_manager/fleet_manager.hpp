#pragma once

#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "cloud/analytics/alert_engine.hpp"
#include "cloud/analytics/health_scoring.hpp"
#include "cloud/analytics/inventory_forecast.hpp"
#include "cloud/analytics/predictive_maintenance.hpp"
#include "cloud/device_twin/models.hpp"

namespace cloud::fleet_manager {

struct FleetMetrics {
  int total_drawers{0};
  int online_drawers{0};
  int unhealthy_drawers{0};
  std::map<std::string, int> firmware_distribution;
  double average_health_score{0.0};
  int active_alarms{0};
};

void to_json(nlohmann::json& j, const FleetMetrics& v);

class FleetManager {
 public:
  void upsert(device_twin::DrawerTwin twin);
  std::vector<device_twin::DrawerTwin> list() const;
  std::optional<device_twin::DrawerTwin> get(const std::string& drawer_id) const;
  FleetMetrics metrics() const;
  void record_history(const std::string& drawer_id, device_twin::HistoryEvent event);

 private:
  device_twin::DrawerTwin enrich(device_twin::DrawerTwin twin) const;

  mutable std::mutex mu_;
  std::unordered_map<std::string, device_twin::DrawerTwin> twins_;
  analytics::HealthScoringEngine health_;
  analytics::PredictiveMaintenanceEngine maintenance_;
  analytics::InventoryForecastEngine inventory_;
  analytics::AlertEngine alerts_;
};

FleetManager& default_manager();
device_twin::DrawerTwin make_local_default_twin();
std::string now_iso();

}  // namespace cloud::fleet_manager
