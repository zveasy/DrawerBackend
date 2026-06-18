#pragma once

#include "cloud/device_twin/models.hpp"

namespace cloud::analytics {

class AlertEngine {
 public:
  std::vector<device_twin::Alert> evaluate(const device_twin::DrawerTwin& twin,
                                           const std::string& now_iso) const;
};

}  // namespace cloud::analytics
