#pragma once

#include <functional>

#include <httplib.h>

#include "edge_platform/service.hpp"

namespace edge_platform {

using AccessCheck = std::function<bool(const httplib::Request&, httplib::Response&)>;

void register_edge_routes(httplib::Server& server, EdgePlatformService& service,
                          AccessCheck access_check = {});

}  // namespace edge_platform
