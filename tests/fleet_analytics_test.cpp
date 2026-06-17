#include <gtest/gtest.h>

#include <algorithm>

#include "cloud/analytics/alert_engine.hpp"
#include "cloud/analytics/health_scoring.hpp"
#include "cloud/analytics/inventory_forecast.hpp"
#include "cloud/analytics/predictive_maintenance.hpp"
#include "cloud/fleet_manager/fleet_manager.hpp"

using cloud::analytics::AlertEngine;
using cloud::analytics::HealthScoringEngine;
using cloud::analytics::InventoryForecastEngine;
using cloud::analytics::PredictiveMaintenanceEngine;
using cloud::device_twin::DenominationInventory;
using cloud::device_twin::DrawerHealth;
using cloud::device_twin::DrawerTwin;
using cloud::device_twin::InventoryState;
using cloud::fleet_manager::FleetManager;

TEST(FleetAnalytics, HealthScorePenalizesKnownFailureSignals) {
  DrawerHealth input;
  input.jam_count = 3;
  input.dispense_failures = 2;
  input.scale_drift_g = 2.4;
  input.self_test_failures = 1;
  input.communication_outages = 1;
  input.hopper_depletion_events = 2;

  auto scored = HealthScoringEngine().score(input);

  EXPECT_LT(scored.score, 60);
  EXPECT_GE(scored.score, 0);
  EXPECT_NE(scored.reasons.end(),
            std::find(scored.reasons.begin(), scored.reasons.end(), "jam_count"));
  EXPECT_NE(scored.reasons.end(),
            std::find(scored.reasons.begin(), scored.reasons.end(), "scale_drift"));
}

TEST(FleetAnalytics, PredictiveMaintenancePromotesCriticalRisk) {
  DrawerHealth input;
  input.motor_cycles = 240000;
  input.jam_count = 12;
  input.scale_drift_g = 4.5;
  input.transaction_latency_ms = 4500;
  input.uptime_percent = 93.0;

  auto assessment = PredictiveMaintenanceEngine().evaluate(input, "2026-01-01T00:00:00Z");

  EXPECT_GE(assessment.predicted_failure_probability, 0.70);
  EXPECT_EQ("critical", assessment.maintenance_priority);
  EXPECT_NE(assessment.drivers.end(),
            std::find(assessment.drivers.begin(), assessment.drivers.end(), "motor_wear"));
}

TEST(FleetAnalytics, InventoryForecastFindsSoonestDepletion) {
  InventoryState state;
  state.denominations["quarter"] = DenominationInventory{"quarter", 24, 400, 4.0, "now"};
  state.denominations["dime"] = DenominationInventory{"dime", 200, 300, 2.0, "now"};

  auto forecast = InventoryForecastEngine().forecast(state);

  EXPECT_DOUBLE_EQ(6.0, forecast.hours_until_empty);
  EXPECT_FALSE(forecast.refill_recommendations.empty());
}

TEST(FleetAnalytics, AlertsCoverJamsCommsInventoryFirmwareAndSelfTest) {
  DrawerTwin twin;
  twin.drawer_id = "drawer-1";
  twin.connectivity_status = "offline";
  twin.health.jam_count = 10;
  twin.health.communication_outages = 5;
  twin.health.self_test_failures = 2;
  twin.firmware.current_version = "1.0.0";
  twin.firmware.target_version = "1.1.0";
  twin.inventory_forecast.hours_until_empty = 4.0;

  auto alerts = AlertEngine().evaluate(twin, "2026-01-01T00:00:00Z");

  EXPECT_GE(alerts.size(), 5u);
  EXPECT_NE(alerts.end(), std::find_if(alerts.begin(), alerts.end(), [](const auto& alert) {
              return alert.type == "low_inventory" && alert.severity == "critical";
            }));
}

TEST(FleetAnalytics, FleetManagerMaintainsHistoryAndAggregateMetrics) {
  FleetManager manager;
  auto twin = cloud::fleet_manager::make_local_default_twin();
  twin.drawer_id = "drawer-history";
  twin.health.jam_count = 10;
  twin.health.dispense_failures = 3;
  twin.health.self_test_failures = 1;
  manager.upsert(twin);

  manager.record_history("drawer-history",
                         {"", "maintenance", "replaced hopper", "", {{"part", "hopper"}}});
  auto loaded = manager.get("drawer-history");
  ASSERT_TRUE(loaded);
  EXPECT_EQ(2u, loaded->history.size());
  EXPECT_LT(loaded->health.score, 70);

  auto metrics = manager.metrics();
  EXPECT_EQ(1, metrics.total_drawers);
  EXPECT_EQ(1, metrics.online_drawers);
  EXPECT_EQ(1, metrics.unhealthy_drawers);
  EXPECT_GT(metrics.active_alarms, 0);
}
