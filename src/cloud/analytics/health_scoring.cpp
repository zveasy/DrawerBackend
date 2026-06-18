#include "cloud/analytics/health_scoring.hpp"

#include <algorithm>
#include <cmath>

namespace cloud::analytics {

device_twin::DrawerHealth HealthScoringEngine::score(
    const device_twin::DrawerHealth& input) const {
  auto out = input;
  out.reasons.clear();
  int penalty = 0;

  auto add = [&](int value, int cap, const std::string& reason) {
    int applied = std::min(value, cap);
    if (applied > 0) {
      penalty += applied;
      out.reasons.push_back(reason);
    }
  };

  add(input.jam_count * 4, 20, "jam_count");
  add(input.dispense_failures * 6, 24, "dispense_failures");
  add(static_cast<int>(std::ceil(std::abs(input.scale_drift_g) * 3.0)), 15, "scale_drift");
  add(input.self_test_failures * 10, 20, "self_test_failures");
  add(input.communication_outages * 5, 15, "communication_outages");
  add(input.hopper_depletion_events * 4, 12, "hopper_depletion_frequency");

  if (input.uptime_percent < 99.0) {
    add(static_cast<int>(std::ceil((99.0 - input.uptime_percent) * 2.0)), 10, "low_uptime");
  }

  out.score = std::clamp(100 - penalty, 0, 100);
  return out;
}

}  // namespace cloud::analytics
