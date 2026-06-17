#pragma once

#include <functional>

#include <httplib.h>

#include "cloud/cash_intelligence/service.hpp"

namespace cloud::cash_intelligence {

using AccessCheck = std::function<bool(const httplib::Request&, httplib::Response&)>;

void register_cash_intelligence_routes(httplib::Server& server, CashIntelligenceService& service,
                                       AccessCheck access_check = {});

}  // namespace cloud::cash_intelligence
