#pragma once

#include <mutex>
#include <optional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "cloud/analytics/alert_engine.hpp"
#include "cloud/analytics/health_scoring.hpp"
#include "cloud/analytics/inventory_forecast.hpp"
#include "cloud/analytics/predictive_maintenance.hpp"
#include "cloud/control_plane/device_twin_control_plane.hpp"
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
  explicit FleetManager(std::string store_path = "",
                        std::shared_ptr<control_plane::DeviceTwinControlPlane> control_plane = {});
  bool submit_update(device_twin::DrawerTwin twin);
  void upsert(device_twin::DrawerTwin twin);
  control_plane::EnrollmentResult enroll(const control_plane::EnrollmentRequest& request);
  std::vector<device_twin::DrawerTwin> list() const;
  std::optional<device_twin::DrawerTwin> get(const std::string& drawer_id) const;
  FleetMetrics metrics() const;
  void record_history(const std::string& drawer_id, device_twin::HistoryEvent event);

 private:
  device_twin::DrawerTwin enrich(device_twin::DrawerTwin twin) const;
  void load_store();
  void save_store_locked() const;
  FleetMetrics metrics_locked() const;
  void publish_metrics_locked(const FleetMetrics& metrics) const;

  mutable std::mutex mu_;
  std::unordered_map<std::string, device_twin::DrawerTwin> twins_;
  std::string store_path_;
  std::shared_ptr<control_plane::DeviceTwinControlPlane> control_plane_;
  analytics::HealthScoringEngine health_;
  analytics::PredictiveMaintenanceEngine maintenance_;
  analytics::InventoryForecastEngine inventory_;
  analytics::AlertEngine alerts_;
};

FleetManager& default_manager();
device_twin::DrawerTwin make_local_default_twin();
std::string now_iso();

}  // namespace cloud::fleet_manager
