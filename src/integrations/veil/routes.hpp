#pragma once

#include <functional>

#include <httplib.h>

#include "integrations/veil/service.hpp"

namespace integrations::veil {

using AccessCheck = std::function<bool(const httplib::Request&, httplib::Response&)>;

void register_trust_routes(httplib::Server& server, VeilTrustService& service,
                           AccessCheck access_check = {});

}  // namespace integrations::veil
