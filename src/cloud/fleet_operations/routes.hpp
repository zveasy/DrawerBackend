#pragma once

#include <functional>

#include <httplib.h>

#include "cloud/fleet_operations/service.hpp"

namespace cloud::fleet_operations {

using AccessCheck = std::function<bool(const httplib::Request&, httplib::Response&)>;

void register_fleet_operations_routes(httplib::Server& server,
                                      FleetOperationsService& service,
                                      AccessCheck access_check = {});

}  // namespace cloud::fleet_operations
