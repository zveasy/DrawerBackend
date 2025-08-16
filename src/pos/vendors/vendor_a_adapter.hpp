#pragma once
#include <string>
#include "../contracts.hpp"

namespace pos { namespace vendors {

// Parse VendorA JSON body and fill PurchaseRequest.  Returns false on error.
bool parse_vendor_a(const std::string &body, const std::string &idem_hdr,
                    PurchaseRequest &out);

} } // namespace pos::vendors

