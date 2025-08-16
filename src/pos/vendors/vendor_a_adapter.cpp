#include "vendor_a_adapter.hpp"
#include <nlohmann/json.hpp>
#include <sstream>

namespace pos { namespace vendors {

static std::string derive_key(const std::string &seed, int price, int deposit) {
  std::string base = seed + "|" + std::to_string(price) + "|" + std::to_string(deposit);
  std::hash<std::string> h;
  std::ostringstream oss;
  oss << "idem-" << std::hex << h(base);
  return oss.str();
}

bool parse_vendor_a(const std::string &body, const std::string &idem_hdr,
                    PurchaseRequest &out) {
  try {
    auto j = nlohmann::json::parse(body);
    out.price_cents = j.at("price_cents").get<int>();
    out.deposit_cents = j.at("deposit_cents").get<int>();
    if (out.price_cents < 0 || out.deposit_cents < 0 ||
        out.price_cents > 100000 || out.deposit_cents > 100000 ||
        out.deposit_cents < out.price_cents)
      return false;
    if (!idem_hdr.empty()) {
      out.idem_key = idem_hdr;
    } else {
      std::string seed = j.value("order_id", "");
      out.idem_key = derive_key(seed, out.price_cents, out.deposit_cents);
    }
    return true;
  } catch (...) {
    return false;
  }
}

} } // namespace pos::vendors

