#pragma once

#include "cloud/device_twin/models.hpp"

namespace cloud::analytics {

class InventoryForecastEngine {
 public:
  device_twin::InventoryForecast forecast(const device_twin::InventoryState& inventory) const;
};

}  // namespace cloud::analytics
