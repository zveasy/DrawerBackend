#include "cloud/analytics/inventory_forecast.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <sstream>

namespace cloud::analytics {

device_twin::InventoryForecast InventoryForecastEngine::forecast(
    const device_twin::InventoryState& inventory) const {
  device_twin::InventoryForecast out;
  double min_hours = std::numeric_limits<double>::infinity();

  for (const auto& item : inventory.denominations) {
    const auto& inv = item.second;
    if (inv.consumption_per_hour > 0.0) {
      double hours = static_cast<double>(std::max(0, inv.quantity)) / inv.consumption_per_hour;
      min_hours = std::min(min_hours, hours);
      if (hours <= 12.0) {
        out.refill_recommendations.push_back("refill " + inv.denomination + " within 12 hours");
      } else if (hours <= 24.0) {
        out.refill_recommendations.push_back("schedule " + inv.denomination + " refill");
      }
      std::ostringstream trend;
      trend << inv.denomination << " depletion at " << inv.consumption_per_hour << "/hour";
      out.depletion_trends.push_back(trend.str());
    }

    if (inv.capacity > 0 && inv.quantity < static_cast<int>(inv.capacity * 0.15)) {
      out.refill_recommendations.push_back("top off low " + inv.denomination + " hopper");
    }
  }

  out.hours_until_empty = std::isinf(min_hours) ? 0.0 : min_hours;
  return out;
}

}  // namespace cloud::analytics
