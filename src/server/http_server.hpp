#pragma once
#include <memory>
#include <atomic>
#include <string>
#include <thread>
#include "../app/txn_engine.hpp"
#include "../app/ishutter.hpp"
#include "../app/idispenser.hpp"

struct StatusSnapshot {
  bool in_progress{false};
  journal::Txn last{};
  std::string version{"1.0-s7"};
};

class HttpServer {
public:
  HttpServer(TxnEngine& engine, IShutter& shutter, IDispenser& dispenser);
  ~HttpServer();
  // Start the server. If cert and key paths are provided, HTTPS is used.
  // Optional token enables Bearer/Basic authentication.
  bool start(const std::string& bind="127.0.0.1", int port=8080,
             const std::string& cert="", const std::string& key="",
             const std::string& token="");
  void stop();
  // for tests
  int port() const { return port_; }
  StatusSnapshot snapshot();
private:
  bool busy_try_acquire();
  void busy_release();
  void setup_routes();
  // impl
  struct Impl; std::unique_ptr<Impl> impl_;
  std::atomic<bool> in_progress_{false};
  std::string auth_key_;
  int port_{8080};
};
