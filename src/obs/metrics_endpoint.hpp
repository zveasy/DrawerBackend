#pragma once
#include <memory>
#include <thread>
#include <string>

namespace httplib { class Server; }

namespace obs {

class MetricsEndpoint {
public:
  MetricsEndpoint();
  ~MetricsEndpoint();
  bool start(const std::string& bind="0.0.0.0", int port=0);
  void stop();
  int port() const { return port_; }
private:
  std::unique_ptr<httplib::Server> server_;
  std::thread th_;
  int port_{0};
};

} // namespace obs

