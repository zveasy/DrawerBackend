#include <gtest/gtest.h>

#include <algorithm>
#include <filesystem>
#include <memory>

#include "cloud/analytics/alert_engine.hpp"
#include "cloud/analytics/health_scoring.hpp"
#include "cloud/analytics/inventory_forecast.hpp"
#include "cloud/analytics/predictive_maintenance.hpp"
#include "cloud/control_plane/device_twin_control_plane.hpp"
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

namespace {

class FakeControlPlane : public cloud::control_plane::DeviceTwinControlPlane {
 public:
  bool fail{false};
  bool conflict{false};
  bool disabled{false};
  int remote_revision{0};
  int disabled_checks{0};
  int pushes{0};

  cloud::control_plane::EnrollmentResult enroll(
      const cloud::control_plane::EnrollmentRequest& request) override {
    DrawerTwin twin = cloud::fleet_manager::make_local_default_twin();
    twin.drawer_id = request.device_id;
    twin.device_id = request.device_id;
    twin.merchant_id = request.merchant_id;
    twin.region = request.region;
    twin.environment = request.environment;
    twin.deployment_channel = request.deployment_channel;
    twin.enrolled = !request.enrollment_token.empty();
    twin.disabled = disabled;
    twin.enrollment_state = twin.enrolled ? "enrolled" : "missing_token";
    return {twin.enrolled && !disabled, disabled, twin.enrollment_state, twin};
  }

  cloud::control_plane::SyncResult push_twin(const DrawerTwin& twin) override {
    ++pushes;
    DrawerTwin remote = twin;
    remote.revision = remote_revision > 0 ? remote_revision : twin.revision;
    if (disabled) return {false, false, true, "device_disabled", remote};
    if (conflict) return {false, true, false, "remote_revision_conflict", remote};
    if (fail) return {false, false, false, "remote_unavailable", remote};
    return {true, false, false, "", remote};
  }

  std::optional<DrawerTwin> fetch_twin(const std::string&) override { return std::nullopt; }
  bool is_device_disabled(const std::string&) override {
    ++disabled_checks;
    return disabled;
  }
};

}  // namespace

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
  state.denominations["quarter"] = DenominationInventory{"quarter", "USD", 24, 400, 4.0, "now"};
  state.denominations["dime"] = DenominationInventory{"dime", "USD", 200, 300, 2.0, "now"};

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

TEST(FleetAnalytics, DeviceTwinPersistsAcrossManagerRestart) {
  auto tmp = std::filesystem::temp_directory_path() / "fleet_twin_store.json";
  std::filesystem::remove(tmp);
  {
    FleetManager manager(tmp.string());
    auto twin = cloud::fleet_manager::make_local_default_twin();
    twin.drawer_id = "drawer-persisted";
    twin.health.jam_count = 4;
    twin.inventory.denominations["quarter"].quantity = 42;
    manager.upsert(twin);
    manager.record_history("drawer-persisted", {"", "inventory_refill", "quarters refilled", "", {}});
  }

  FleetManager restarted(tmp.string());
  auto loaded = restarted.get("drawer-persisted");
  ASSERT_TRUE(loaded);
  EXPECT_EQ("drawer-persisted", loaded->drawer_id);
  EXPECT_EQ(42, loaded->inventory.denominations["quarter"].quantity);
  ASSERT_FALSE(loaded->history.empty());
  EXPECT_EQ("inventory_refill", loaded->history.back().type);
}

TEST(FleetAnalytics, ControlPlaneSyncSuccessFailureConflictAndRevocation) {
  auto control = std::make_shared<FakeControlPlane>();
  FleetManager manager("", control);
  auto twin = cloud::fleet_manager::make_local_default_twin();
  twin.drawer_id = "drawer-sync";
  twin.device_id = "device-sync";
  EXPECT_TRUE(manager.submit_update(twin));
  auto synced = manager.get("drawer-sync");
  ASSERT_TRUE(synced);
  EXPECT_EQ("synced", synced->sync_status);
  EXPECT_FALSE(synced->conflict);

  control->fail = true;
  EXPECT_TRUE(manager.submit_update(twin));
  auto failed = manager.get("drawer-sync");
  ASSERT_TRUE(failed);
  EXPECT_EQ("sync_failed", failed->sync_status);

  control->fail = false;
  control->conflict = true;
  control->remote_revision = 99;
  EXPECT_FALSE(manager.submit_update(twin));
  auto conflicted = manager.get("drawer-sync");
  ASSERT_TRUE(conflicted);
  EXPECT_EQ("conflict", conflicted->sync_status);
  EXPECT_TRUE(conflicted->conflict);
  EXPECT_EQ(99, conflicted->remote_revision);

  control->conflict = false;
  control->disabled = true;
  auto revoked = cloud::fleet_manager::make_local_default_twin();
  revoked.drawer_id = "drawer-sync";
  revoked.device_id = "device-sync";
  int pushes_before_revocation = control->pushes;
  EXPECT_FALSE(manager.submit_update(revoked));
  EXPECT_GT(control->disabled_checks, 0);
  EXPECT_EQ(pushes_before_revocation, control->pushes);
  auto disabled = manager.get("drawer-sync");
  ASSERT_TRUE(disabled);
  EXPECT_TRUE(disabled->disabled);
  EXPECT_EQ("disabled", disabled->sync_status);
}

TEST(FleetAnalytics, EnrollmentCarriesInternationalDeploymentMetadata) {
  auto control = std::make_shared<FakeControlPlane>();
  FleetManager manager("", control);
  auto result = manager.enroll({"device-ke-1", "merchant-7", "KE-NBO", "pilot", "pilot", "token"});
  ASSERT_TRUE(result.ok);
  auto twin = manager.get("device-ke-1");
  ASSERT_TRUE(twin);
  EXPECT_EQ("device-ke-1", twin->device_id);
  EXPECT_EQ("merchant-7", twin->merchant_id);
  EXPECT_EQ("KE-NBO", twin->region);
  EXPECT_EQ("pilot", twin->environment);
  EXPECT_EQ("pilot", twin->deployment_channel);
  EXPECT_TRUE(twin->enrolled);
}

TEST(FleetAnalytics, CurrencyCodesAreValidatedAndPersisted) {
  EXPECT_TRUE(cloud::device_twin::valid_currency_code("USD"));
  EXPECT_TRUE(cloud::device_twin::valid_currency_code("KES"));
  EXPECT_FALSE(cloud::device_twin::valid_currency_code("usd"));
  EXPECT_FALSE(cloud::device_twin::valid_currency_code("US"));
}
