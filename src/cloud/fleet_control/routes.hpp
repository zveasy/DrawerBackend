#pragma once

#include <functional>

#include <httplib.h>

#include "cloud/fleet_control/control_plane.hpp"

namespace cloud::fleet_control {

using AccessCheck = std::function<bool(const httplib::Request&, httplib::Response&)>;

void register_fleet_control_routes(httplib::Server& server, FleetControlPlane& control,
                                   AccessCheck access_check = {});
FleetControlPlane& default_control_plane();

}  // namespace cloud::fleet_control
