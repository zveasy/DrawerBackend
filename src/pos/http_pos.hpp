#pragma once
#include "connector.hpp"
#include <httplib.h>
#include <thread>

namespace pos {

class HttpConnector : public Connector {
public:
  HttpConnector(TxnEngine& eng, const Options& opt = Options());
  ~HttpConnector() override;
  bool start() override;
  void stop() override;
  int port() const { return port_; }
private:
  void setup_routes();
  bool busy_try_acquire();
  void busy_release();
  TxnEngine& eng_;
  Options opt_;
  httplib::Server server_;
  std::thread th_;
  std::atomic<bool> busy_{false};
  int port_{0};
};

} // namespace pos
