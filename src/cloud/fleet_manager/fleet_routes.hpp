#pragma once

#include <httplib.h>

#include "cloud/fleet_manager/fleet_manager.hpp"

namespace cloud::fleet_manager {

void register_fleet_routes(httplib::Server& server, FleetManager& manager);

}  // namespace cloud::fleet_manager
