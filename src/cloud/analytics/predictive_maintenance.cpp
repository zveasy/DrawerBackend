#include "cloud/analytics/predictive_maintenance.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace cloud::analytics {
namespace {

std::string iso_days_from_now(int days) {
  auto tp = std::chrono::system_clock::now() + std::chrono::hours(24 * days);
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

double clamp01(double v) { return std::clamp(v, 0.0, 1.0); }

}  // namespace

device_twin::MaintenanceAssessment PredictiveMaintenanceEngine::evaluate(
    const device_twin::DrawerHealth& health, const std::string&) const {
  double motor = clamp01(static_cast<double>(health.motor_cycles) / 250000.0);
  double jams = clamp01(static_cast<double>(health.jam_count) / 12.0);
  double drift = clamp01(std::abs(health.scale_drift_g) / 5.0);
  double latency = clamp01(health.transaction_latency_ms / 5000.0);
  double uptime = clamp01((100.0 - health.uptime_percent) / 10.0);

  double probability = clamp01((motor * 0.25) + (jams * 0.25) + (drift * 0.20) +
                               (latency * 0.15) + (uptime * 0.15));

  device_twin::MaintenanceAssessment out;
  out.predicted_failure_probability = std::round(probability * 1000.0) / 1000.0;
  if (probability >= 0.70) {
    out.maintenance_priority = "critical";
    out.recommended_service_date = iso_days_from_now(1);
  } else if (probability >= 0.40) {
    out.maintenance_priority = "warning";
    out.recommended_service_date = iso_days_from_now(7);
  } else {
    out.maintenance_priority = "low";
    out.recommended_service_date = iso_days_from_now(30);
  }

  if (motor >= 0.5) out.drivers.push_back("motor_wear");
  if (jams >= 0.5) out.drivers.push_back("jam_frequency");
  if (drift >= 0.5) out.drivers.push_back("scale_calibration_drift");
  if (latency >= 0.5) out.drivers.push_back("transaction_latency");
  if (uptime >= 0.2) out.drivers.push_back("uptime_percentage");
  return out;
}

}  // namespace cloud::analytics
