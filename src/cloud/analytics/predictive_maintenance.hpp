#pragma once

#include "cloud/device_twin/models.hpp"

namespace cloud::analytics {

class PredictiveMaintenanceEngine {
 public:
  device_twin::MaintenanceAssessment evaluate(const device_twin::DrawerHealth& health,
                                              const std::string& now_iso) const;
};

}  // namespace cloud::analytics
