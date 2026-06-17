#include "http_server.hpp"
#include <httplib.h>
#include <sstream>
#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <mutex>
#include <string>
#include <nlohmann/json.hpp>
#include "obs/metrics.hpp"
#include "server/version_endpoint.hpp"
#include "server/docs_endpoint.hpp"
#include "cloud/fleet_manager/fleet_routes.hpp"
#include "ops/operational_status.hpp"
#include "util/log.hpp"

namespace {

bool env_true(const char* name) {
  const char* value = std::getenv(name);
  return value && (std::string(value) == "1" || std::string(value) == "true" ||
                   std::string(value) == "TRUE" || std::string(value) == "yes");
}

bool production_mode() {
  const char* env = std::getenv("REGISTER_MVP_ENV");
  const char* node_env = std::getenv("NODE_ENV");
  return env_true("REGISTER_MVP_PRODUCTION") ||
         (env && std::string(env) == "production") ||
         (node_env && std::string(node_env) == "production");
}

bool is_loopback_bind(const std::string& bind) {
  return bind.empty() || bind == "127.0.0.1" || bind == "localhost" || bind == "::1";
}

bool is_health_path(const std::string& path) {
  return path == "/healthz";
}

bool is_protected_path(const std::string& path) {
  if (is_health_path(path)) return false;
  return true;
}

}  // namespace

struct HttpServer::Impl {
  std::unique_ptr<httplib::Server> server;
  TxnEngine& engine;
  IShutter& shutter;
  IDispenser& dispenser;
  std::thread th;
  Impl(TxnEngine& e, IShutter& s, IDispenser& d) : engine(e), shutter(s), dispenser(d) {}
  struct RateLimiter {
    double capacity;
    double tokens;
    double refill_per_sec;
    std::chrono::steady_clock::time_point last;
    std::mutex m;
    RateLimiter(double cap, double refill)
        : capacity(cap), tokens(cap), refill_per_sec(refill),
          last(std::chrono::steady_clock::now()) {}
    bool allow() {
      std::lock_guard<std::mutex> lk(m);
      auto now = std::chrono::steady_clock::now();
      double elapsed =
          std::chrono::duration<double>(now - last).count();
      tokens = std::min(capacity, tokens + elapsed * refill_per_sec);
      last = now;
      if (tokens >= 1.0) {
        tokens -= 1.0;
        return true;
      }
      return false;
    }
  };
  RateLimiter txn_limiter{4.0, 0.1};
  RateLimiter cmd_limiter{4.0, 0.1};
  bool allow_unauth_loopback{false};
};

HttpServer::HttpServer(TxnEngine& engine, IShutter& shutter, IDispenser& dispenser)
    : impl_(new Impl(engine, shutter, dispenser)) {}

HttpServer::~HttpServer() { stop(); }

bool HttpServer::start(const std::string& bind, int port, const std::string& cert,
                       const std::string& key, const std::string& token) {
  if (impl_->th.joinable()) return false;
  auth_key_ = token;
  if (auth_key_.empty()) {
    const char* env_token = std::getenv("REGISTER_MVP_API_TOKEN");
    if (env_token) auth_key_ = env_token;
  }
  bool has_auth = !auth_key_.empty();
  bool loopback = is_loopback_bind(bind);
  bool prod = production_mode();
  impl_->allow_unauth_loopback = !has_auth && loopback && !prod;
  if (!has_auth && (prod || !loopback)) {
    LOG_ERROR("api_auth_config_unsafe",
              {{"bind", bind}, {"production", prod ? "1" : "0"}, {"reason", "missing_token"}});
    return false;
  }
  if (!cert.empty() && !key.empty()) {
    impl_->server = std::make_unique<httplib::SSLServer>(cert.c_str(), key.c_str());
  } else {
    impl_->server = std::make_unique<httplib::Server>();
  }
  setup_routes();
  int actual = 0;
  if (port == 0) {
    actual = impl_->server->bind_to_any_port(bind.c_str());
    if (actual <= 0) return false;
  } else {
    if (!impl_->server->bind_to_port(bind.c_str(), port)) return false;
    actual = port;
  }
  port_ = actual;
  impl_->th = std::thread([this]() { impl_->server->listen_after_bind(); });
  impl_->server->wait_until_ready();
  return true;
}

void HttpServer::stop() {
  if (impl_->server) impl_->server->stop();
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

void HttpServer::setup_routes() {
  auto& svr = *impl_->server;
  server::register_version_routes(svr);
  server::register_docs_routes(svr);

  std::string token = auth_key_;
  std::string basic;
  if (!token.empty()) {
    basic = "Basic " + [](const std::string& in) {
      static const char table[] =
          "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
      std::string out;
      int val = 0, valb = -6;
      for (unsigned char c : in) {
        val = (val << 8) + c;
        valb += 8;
        while (valb >= 0) {
          out.push_back(table[(val >> valb) & 0x3F]);
          valb -= 6;
        }
      }
      if (valb > -6) out.push_back(table[((val << 8) >> (valb + 8)) & 0x3F]);
      while (out.size() % 4) out.push_back('=');
      return out;
    }(":" + token);
  }
  auto authorize = [this, token, basic](const httplib::Request& req, httplib::Response& res) {
    if (!is_protected_path(req.path)) return true;
    if (impl_->allow_unauth_loopback) return true;
    auto auth = req.get_header_value("Authorization");
    if (!token.empty() && (auth == ("Bearer " + token) || auth == basic)) return true;
    res.status = 401;
    res.set_header("WWW-Authenticate", "Basic realm=\"\"");
    res.set_content("{\"error\":\"unauthorized\"}", "application/json");
    util::log(util::LogLevel::Warn, "http_auth_failure",
              {{"src", req.remote_addr}, {"route", req.path}, {"status", "401"}});
    return false;
  };
  cloud::fleet_manager::register_fleet_routes(svr, cloud::fleet_manager::default_manager(), authorize);

  svr.set_pre_routing_handler([this, authorize](const httplib::Request& req,
                                               httplib::Response& res) {
    auto log_err = [&](const std::string& reason) {
      if (req.path == "/txn" || req.path == "/command") {
        auto lvl = res.status >= 500 ? util::LogLevel::Error : util::LogLevel::Warn;
        util::log(lvl, "http_error",
                  {{"src", req.remote_addr},
                   {"route", req.path},
                   {"status", std::to_string(res.status)},
                   {"reason", reason}});
      }
    };
    if (req.path == "/txn") {
      if (!impl_->txn_limiter.allow()) {
        res.status = 429;
        res.set_content("{\"error\":\"rate\"}", "application/json");
        log_err("rate");
        return httplib::Server::HandlerResponse::Handled;
      }
    } else if (req.path == "/command") {
      if (!impl_->cmd_limiter.allow()) {
        res.status = 429;
        res.set_content("{\"error\":\"rate\"}", "application/json");
        log_err("rate");
        return httplib::Server::HandlerResponse::Handled;
      }
    }
    if (req.path == "/txn" || req.path == "/command") {
      auto local = cloud::fleet_manager::default_manager().get("local-drawer");
      if (local && local->disabled) {
        res.status = 423;
        res.set_content("{\"error\":\"device_disabled\"}", "application/json");
        util::log(util::LogLevel::Warn, "device_command_blocked",
                  {{"src", req.remote_addr}, {"route", req.path}, {"reason", "disabled"}});
        return httplib::Server::HandlerResponse::Handled;
      }
    }
    if (!authorize(req, res)) {
      log_err("unauthorized");
      return httplib::Server::HandlerResponse::Handled;
    }
    return httplib::Server::HandlerResponse::Unhandled;
  });
  svr.Post("/txn", [this](const httplib::Request& req, httplib::Response& res) {
    struct Guard {
      HttpServer* s;
      bool ok;
      Guard(HttpServer* s_) : s(s_), ok(s_->busy_try_acquire()) {}
      ~Guard() {
        if (ok) s->busy_release();
      }
    } guard(this);
    if (!guard.ok) {
      res.status = 409;
      res.set_content("{\"error\":\"busy\"}", "application/json");
      util::log(util::LogLevel::Warn, "http_error",
                {{"src", req.remote_addr},
                 {"route", req.path},
                 {"status", std::to_string(res.status)},
                 {"reason", "busy"}});
      return;
    }
    int price = 0, deposit = 0;
    try {
      auto j = nlohmann::json::parse(req.body);
      price = j.at("price").get<int>();
      deposit = j.at("deposit").get<int>();
    } catch (...) {
      res.status = 400;
      res.set_content("{\"error\":\"bad\"}", "application/json");
      util::log(util::LogLevel::Warn, "http_error",
                {{"src", req.remote_addr},
                 {"route", req.path},
                 {"status", std::to_string(res.status)},
                 {"reason", "bad_json"}});
      return;
    }
    price = std::clamp(price, 0, 100000);
    deposit = std::clamp(deposit, 0, 100000);
    auto t = impl_->engine.run_purchase(price, deposit);
    std::ostringstream oss;
    oss << "{\"id\":\"" << t.id << "\",\"price\":" << t.price << ",\"deposit\":" << t.deposit
        << ",\"change\":" << t.change << ",\"quarters\":" << t.quarters << ",\"status\":\""
        << (t.phase == "DONE" ? "OK" : "VOID") << "\",\"reason\":\"" << t.reason << "\"}";
    res.set_content(oss.str(), "application/json");
  });

  svr.Get("/status", [this](const httplib::Request&, httplib::Response& res) {
    auto snap = snapshot();
    bool has_last = !snap.last.id.empty();
    std::string last_json = has_last ? journal::to_json(snap.last) : std::string("{}");
    nlohmann::json status = {{"in_progress", snap.in_progress},
                             {"last", has_last ? nlohmann::json::parse(last_json) : nlohmann::json::object()},
                             {"version", snap.version}};
    status["operational"] = ops::current_status();
    res.set_content(status.dump(), "application/json");
  });

  svr.Post("/command", [this](const httplib::Request& req, httplib::Response& res) {
    int val = 0;
    nlohmann::json j;
    try {
      j = nlohmann::json::parse(req.body);
    } catch (...) {
      res.status = 400;
      res.set_content("{\"ok\":false,\"reason\":\"bad\"}", "application/json");
      util::log(util::LogLevel::Warn, "http_error",
                {{"src", req.remote_addr},
                 {"route", req.path},
                 {"status", std::to_string(res.status)},
                 {"reason", "bad_json"}});
      return;
    }
    if (j.contains("close")) {
      try {
        val = j.at("close").get<int>();
      } catch (...) {
        res.status = 400;
        res.set_content("{\"ok\":false,\"reason\":\"bad\"}", "application/json");
        util::log(util::LogLevel::Warn, "http_error",
                  {{"src", req.remote_addr},
                   {"route", req.path},
                   {"status", std::to_string(res.status)},
                   {"reason", "bad_request"}});
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
      util::log(util::LogLevel::Warn, "http_error",
                {{"src", req.remote_addr},
                 {"route", req.path},
                 {"status", std::to_string(res.status)},
                 {"reason", "busy"}});
      return;
    }
    if (j.contains("open")) {
      try {
        val = j.at("open").get<int>();
      } catch (...) {
        res.status = 400;
        res.set_content("{\"ok\":false,\"reason\":\"bad\"}", "application/json");
        util::log(util::LogLevel::Warn, "http_error",
                  {{"src", req.remote_addr},
                   {"route", req.path},
                   {"status", std::to_string(res.status)},
                   {"reason", "bad_request"}});
        return;
      }
      val = std::clamp(val, 0, 1000);
      impl_->shutter.open_mm(val, nullptr);
      res.set_content("{\"ok\":true}", "application/json");
      return;
    }
    if (j.contains("dispense")) {
      try {
        val = j.at("dispense").get<int>();
      } catch (...) {
        res.status = 400;
        res.set_content("{\"ok\":false,\"reason\":\"bad\"}", "application/json");
        util::log(util::LogLevel::Warn, "http_error",
                  {{"src", req.remote_addr},
                   {"route", req.path},
                   {"status", std::to_string(res.status)},
                   {"reason", "bad_request"}});
        return;
      }
      val = std::clamp(val, 0, 1000);
      impl_->dispenser.dispenseCoins(val);
      res.set_content("{\"ok\":true}", "application/json");
      return;
    }
    res.status = 400;
    res.set_content("{\"ok\":false,\"reason\":\"bad\"}", "application/json");
    util::log(util::LogLevel::Warn, "http_error",
              {{"src", req.remote_addr},
               {"route", req.path},
               {"status", std::to_string(res.status)},
               {"reason", "bad_request"}});
  });

  svr.Get("/metrics", [](const httplib::Request&, httplib::Response& res) {
    std::ostringstream oss;
    obs::M().to_prometheus(oss);
    res.set_content(oss.str(), "text/plain");
  });
  svr.Get("/metrics.json", [](const httplib::Request&, httplib::Response& res) {
    std::ostringstream oss;
    obs::M().to_json(oss);
    res.set_content(oss.str(), "application/json");
  });
}
