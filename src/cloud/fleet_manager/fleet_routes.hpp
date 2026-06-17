#pragma once

#include <httplib.h>
#include <functional>

#include "cloud/fleet_manager/fleet_manager.hpp"

namespace cloud::fleet_manager {

using AccessCheck = std::function<bool(const httplib::Request&, httplib::Response&)>;

void register_fleet_routes(httplib::Server& server, FleetManager& manager,
                           AccessCheck access_check = {});

}  // namespace cloud::fleet_manager
