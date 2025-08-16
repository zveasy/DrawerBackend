#include "http_pos.hpp"
#include <sstream>
#include <algorithm>

namespace pos {

HttpConnector::HttpConnector(TxnEngine& eng, const Options& opt)
    : eng_(eng), opt_(opt) {}

HttpConnector::~HttpConnector() { stop(); }

bool HttpConnector::start() {
  if (th_.joinable()) return false;
  setup_routes();
  int actual = 0;
  if (opt_.port == 0) {
    actual = server_.bind_to_any_port(opt_.bind.c_str());
    if (actual <= 0) return false;
  } else {
    if (!server_.bind_to_port(opt_.bind.c_str(), opt_.port)) return false;
    actual = opt_.port;
  }
  port_ = actual;
  th_ = std::thread([this]() { server_.listen_after_bind(); });
  server_.wait_until_ready();
  return true;
}

void HttpConnector::stop() {
  server_.stop();
  if (th_.joinable()) th_.join();
}

bool HttpConnector::busy_try_acquire() { return !busy_.exchange(true); }
void HttpConnector::busy_release() { busy_.store(false); }

static bool get_int_field(const std::string& body, const std::string& key, int& value) {
  auto pos = body.find("\"" + key + "\"");
  if (pos == std::string::npos) return false;
  pos = body.find(':', pos);
  if (pos == std::string::npos) return false;
  pos = body.find_first_of("-0123456789", pos + 1);
  if (pos == std::string::npos) return false;
  size_t end = body.find_first_not_of("0123456789-", pos);
  try {
    value = std::stoi(body.substr(pos, end - pos));
  } catch (...) {
    return false;
  }
  return true;
}

void HttpConnector::setup_routes() {
  server_.Post("/purchase", [this](const httplib::Request& req, httplib::Response& res) {
    if (!opt_.shared_key.empty()) {
      auto hdr = req.get_header_value("X-Pos-Key");
      if (hdr != opt_.shared_key) {
        res.status = 401;
        res.set_content("{\"error\":\"unauthorized\"}", "application/json");
        return;
      }
    }
    struct Guard {
      HttpConnector* s; bool ok; Guard(HttpConnector* s_) : s(s_), ok(s_->busy_try_acquire()) {}
      ~Guard() { if (ok) s->busy_release(); }
    } guard(this);
    if (!guard.ok) {
      res.status = 409;
      res.set_content("{\"error\":\"busy\"}", "application/json");
      return;
    }
    int price=0, deposit=0;
    if (!get_int_field(req.body, "price", price) ||
        !get_int_field(req.body, "deposit", deposit)) {
      res.status = 400;
      res.set_content("{\"error\":\"bad_request\"}", "application/json");
      return;
    }
    price = std::clamp(price, 0, 100000);
    deposit = std::clamp(deposit, 0, 100000);
    auto t = eng_.run_purchase(price, deposit);
    std::ostringstream oss;
    oss << "{\"status\":\"" << (t.phase=="DONE"?"OK":"VOID") << "\""
        << ",\"id\":\"" << t.id << "\""
        << ",\"quarters\":" << t.quarters
        << ",\"change\":" << t.change << "}";
    res.set_content(oss.str(), "application/json");
  });

  server_.Get("/ping", [this](const httplib::Request&, httplib::Response& res) {
    res.set_content("{\"pong\":true,\"version\":\"1.0-s12\"}", "application/json");
  });
}

} // namespace pos
