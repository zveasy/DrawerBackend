#include "http_server.hpp"
#include <httplib.h>
#include <sstream>
#include <algorithm>
#include <chrono>
#include "obs/metrics.hpp"

struct HttpServer::Impl {
  httplib::Server server;
  TxnEngine& engine;
  IShutter& shutter;
  IDispenser& dispenser;
  std::thread th;
  Impl(TxnEngine& e, IShutter& s, IDispenser& d)
      : engine(e), shutter(s), dispenser(d) {}
};

HttpServer::HttpServer(TxnEngine& engine, IShutter& shutter, IDispenser& dispenser)
    : impl_(new Impl(engine, shutter, dispenser)) {}

HttpServer::~HttpServer() { stop(); }

bool HttpServer::start(const std::string& bind, int port) {
  if (impl_->th.joinable()) return false;
  setup_routes();
  int actual = 0;
  if (port == 0) {
    actual = impl_->server.bind_to_any_port(bind.c_str());
    if (actual <= 0) return false;
  } else {
    if (!impl_->server.bind_to_port(bind.c_str(), port)) return false;
    actual = port;
  }
  port_ = actual;
  impl_->th = std::thread([this]() { impl_->server.listen_after_bind(); });
  impl_->server.wait_until_ready();
  return true;
}

void HttpServer::stop() {
  impl_->server.stop();
  if (impl_->th.joinable()) impl_->th.join();
}

bool HttpServer::busy_try_acquire() { return !in_progress_.exchange(true); }

void HttpServer::busy_release() { in_progress_.store(false); }

StatusSnapshot HttpServer::snapshot() {
  StatusSnapshot s;
  s.in_progress = in_progress_.load();
  journal::load_last(s.last);
  return s;
}

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

void HttpServer::setup_routes() {
  auto& svr = impl_->server;
  svr.Post("/txn", [this](const httplib::Request& req, httplib::Response& res) {
    struct Guard {
      HttpServer* s; bool ok; Guard(HttpServer* s_) : s(s_), ok(s_->busy_try_acquire()) {}
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
      res.set_content("{\"error\":\"bad\"}", "application/json");
      return;
    }
    price = std::clamp(price, 0, 100000);
    deposit = std::clamp(deposit, 0, 100000);
    auto t = impl_->engine.run_purchase(price, deposit);
    std::ostringstream oss;
    oss << "{\"id\":\"" << t.id << "\",\"price\":" << t.price
        << ",\"deposit\":" << t.deposit
        << ",\"change\":" << t.change
        << ",\"quarters\":" << t.quarters
        << ",\"status\":\"" << (t.phase=="DONE"?"OK":"VOID")
        << "\",\"reason\":\"" << t.reason << "\"}";
    res.set_content(oss.str(), "application/json");
  });

  svr.Get("/status", [this](const httplib::Request&, httplib::Response& res) {
    auto snap = snapshot();
    bool has_last = !snap.last.id.empty();
    std::string last_json = has_last ? journal::to_json(snap.last) : std::string("{}");
    std::ostringstream oss;
    oss << "{\"in_progress\":" << (snap.in_progress?"true":"false")
        << ",\"last\":" << (has_last?last_json:"{}")
        << ",\"version\":\"" << snap.version << "\"}";
    res.set_content(oss.str(), "application/json");
  });

  svr.Post("/command", [this](const httplib::Request& req, httplib::Response& res) {
    int val=0;
    const auto& body = req.body;
    if (body.find("\"close\"") != std::string::npos) {
      if (!get_int_field(body, "close", val)) {
        res.status = 400;
        res.set_content("{\"ok\":false,\"reason\":\"bad\"}", "application/json");
        return;
      }
      val = std::clamp(val, 0, 1000);
      impl_->shutter.close_mm(val, nullptr);
      res.set_content("{\"ok\":true}", "application/json");
      return;
    }
    if (in_progress_.load()) {
      res.status = 409;
      res.set_content("{\"error\":\"busy\"}", "application/json");
      return;
    }
    if (body.find("\"open\"") != std::string::npos) {
      if (!get_int_field(body, "open", val)) {
        res.status = 400;
        res.set_content("{\"ok\":false,\"reason\":\"bad\"}", "application/json");
        return;
      }
      val = std::clamp(val, 0, 1000);
      impl_->shutter.open_mm(val, nullptr);
      res.set_content("{\"ok\":true}", "application/json");
      return;
    }
    if (body.find("\"dispense\"") != std::string::npos) {
      if (!get_int_field(body, "dispense", val)) {
        res.status = 400;
        res.set_content("{\"ok\":false,\"reason\":\"bad\"}", "application/json");
        return;
      }
      val = std::clamp(val, 0, 1000);
      impl_->dispenser.dispenseCoins(val);
      res.set_content("{\"ok\":true}", "application/json");
      return;
    }
    res.status = 400;
    res.set_content("{\"ok\":false,\"reason\":\"bad\"}", "application/json");
  });

  svr.Get("/metrics", [](const httplib::Request&, httplib::Response& res){
    std::ostringstream oss; obs::M().to_prometheus(oss);
    res.set_content(oss.str(), "text/plain");
  });
  svr.Get("/metrics.json", [](const httplib::Request&, httplib::Response& res){
    std::ostringstream oss; obs::M().to_json(oss);
    res.set_content(oss.str(), "application/json");
  });
}
