#pragma once
#include "connector.hpp"
#include "router.hpp"
#include "vendors/vendor_a_adapter.hpp"
#include "vendors/vendor_b_adapter.hpp"
#include <httplib.h>
#include <thread>

namespace pos {

class HttpConnector : public Connector {
public:
  HttpConnector(Router &router, const Options &opt = Options());
  ~HttpConnector() override;
  bool start() override;
  void stop() override;
  int port() const { return port_; }

private:
  void setup_routes();
  Router &router_;
  Options opt_;
  httplib::Server server_;
  std::thread th_;
  int port_{0};
};

} // namespace pos
