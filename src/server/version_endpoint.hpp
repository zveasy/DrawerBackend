#pragma once
#include <httplib.h>

namespace server {
/**
 * Register /version and /healthz endpoints for the ops API.
 */
void register_version_routes(httplib::Server& svr);
}
