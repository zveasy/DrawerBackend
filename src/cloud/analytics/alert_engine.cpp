#include "cloud/analytics/alert_engine.hpp"

namespace cloud::analytics {

std::vector<device_twin::Alert> AlertEngine::evaluate(const device_twin::DrawerTwin& twin,
                                                      const std::string& now_iso) const {
  std::vector<device_twin::Alert> out;
  auto add = [&](const std::string& type, const std::string& severity,
                 const std::string& message) {
    out.push_back(device_twin::Alert{"alert-" + twin.drawer_id + "-" + type, type, severity,
                                     message, now_iso});
  };

  if (twin.health.jam_count >= 6) {
    add("repeated_jams", twin.health.jam_count >= 10 ? "critical" : "warning",
        "jam frequency exceeds fleet threshold");
  }
  if (twin.health.communication_outages >= 3 || twin.connectivity_status != "online") {
    add("communication_failure",
        twin.health.communication_outages >= 5 || twin.connectivity_status == "offline" ? "critical"
                                                                                        : "warning",
        "device connectivity is degraded");
  }
  if (twin.health.self_test_failures > 0) {
    add("self_test_failure", twin.health.self_test_failures >= 2 ? "critical" : "warning",
        "recent self-test failure reported");
  }
  if (twin.firmware.current_version != twin.firmware.target_version) {
    add("firmware_mismatch", "warning", "firmware does not match target version");
  }
  if (twin.inventory_forecast.hours_until_empty > 0.0 &&
      twin.inventory_forecast.hours_until_empty <= 24.0) {
    add("low_inventory", twin.inventory_forecast.hours_until_empty <= 8.0 ? "critical" : "warning",
        "one or more denominations are approaching depletion");
  }
  return out;
}

}  // namespace cloud::analytics
