#pragma once

#include "cloud/device_twin/models.hpp"

namespace cloud::analytics {

class HealthScoringEngine {
 public:
  device_twin::DrawerHealth score(const device_twin::DrawerHealth& input) const;
};

}  // namespace cloud::analytics
