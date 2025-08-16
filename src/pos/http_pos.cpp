#include "http_pos.hpp"
#include "contracts.hpp"
#include <sstream>

namespace pos {

HttpConnector::HttpConnector(Router &router, const Options &opt)
    : router_(router), opt_(opt) {}

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

void HttpConnector::setup_routes() {
  server_.Post("/pos/purchase", [this](const httplib::Request& req, httplib::Response& res) {
    if (!opt_.shared_key.empty()) {
      auto hdr = req.get_header_value("X-Pos-Key");
      if (hdr != opt_.shared_key) {
        res.status = 401;
        res.set_content("{\"error\":\"unauthorized\"}", "application/json");
        return;
      }
    }

    PurchaseRequest preq;
    std::string idem = req.get_header_value("X-Idempotency-Key");
    bool ok=false;
    if (opt_.vendor_mode == "B")
      ok = vendors::parse_vendor_b(req.body, idem, preq);
    else
      ok = vendors::parse_vendor_a(req.body, idem, preq);
    if (!ok || preq.idem_key.empty()) {
      res.status = 400;
      res.set_content(bad_request_json(), "application/json");
      return;
    }

    auto out = router_.handle(preq);
    res.status = out.first;
    res.set_content(out.second, "application/json");
  });

  server_.Get("/ping", [this](const httplib::Request&, httplib::Response& res) {
    res.set_content("{\"pong\":true,\"version\":\"1.0-s18\"}", "application/json");
  });
}

} // namespace pos

