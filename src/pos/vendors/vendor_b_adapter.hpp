#pragma once
#include <string>
#include "../contracts.hpp"

namespace pos { namespace vendors {

bool parse_vendor_b(const std::string &body, const std::string &idem_hdr,
                    PurchaseRequest &out);

} } // namespace pos::vendors

