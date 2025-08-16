#include "serial_pos.hpp"
#include "contracts.hpp"
#include <sstream>
#include <nlohmann/json.hpp>

namespace pos {

SerialConnector::SerialConnector(Router &router, const Options &opt)
    : router_(router), opt_(opt) {}

bool SerialConnector::start() { return true; }
void SerialConnector::stop() {}

static bool parse_req(const std::string &line, PurchaseRequest &req) {
  std::istringstream iss(line);
  std::string token;
  bool seen=false;
  while (iss >> token) {
    auto eq = token.find('=');
    if (eq == std::string::npos) continue;
    auto k = token.substr(0, eq);
    auto v = token.substr(eq + 1);
    if (k == "id") req.idem_key = v;
    else if (k == "price") req.price_cents = std::stoi(v);
    else if (k == "deposit") req.deposit_cents = std::stoi(v);
    seen = true;
  }
  if (!seen) return false;
  if (req.idem_key.empty() || req.price_cents < 0 || req.deposit_cents < 0 ||
      req.price_cents > 100000 || req.deposit_cents > 100000 ||
      req.deposit_cents < req.price_cents)
    return false;
  return true;
}

std::string SerialConnector::handle_line(const std::string &line) {
  PurchaseRequest req;
  if (!parse_req(line, req)) {
    if (req.idem_key.empty()) return "BAD id=";
    return "BAD id=" + req.idem_key;
  }

  auto out = router_.handle(req);
  if (out.first == 200) {
    auto j = nlohmann::json::parse(out.second);
    int change = j["change_cents"].get<int>();
    int q = j["coins"]["quarter"].get<int>();
    int d = j["coins"]["dime"].get<int>();
    int n = j["coins"]["nickel"].get<int>();
    int p = j["coins"]["penny"].get<int>();
    std::ostringstream oss;
    oss << "OK id=" << req.idem_key << " change=" << change
        << " q=" << q << " d=" << d << " n=" << n << " p=" << p;
    return oss.str();
  } else if (out.first == 409) {
    if (out.second.find("busy") != std::string::npos)
      return "BUSY id=" + req.idem_key;
    return "CONFLICT id=" + req.idem_key;
  } else if (out.first == 202) {
    return "PENDING id=" + req.idem_key;
  }
  return "BAD id=" + req.idem_key;
}

} // namespace pos

