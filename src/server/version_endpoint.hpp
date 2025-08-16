#pragma once
#include <httplib.h>

namespace server {
void register_version_routes(httplib::Server& svr);
}

