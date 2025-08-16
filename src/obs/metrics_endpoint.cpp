#include "obs/metrics_endpoint.hpp"
#include "obs/metrics.hpp"
#include <httplib.h>
#include <sstream>

namespace obs {

MetricsEndpoint::MetricsEndpoint() = default;
MetricsEndpoint::~MetricsEndpoint() { stop(); }

bool MetricsEndpoint::start(const std::string& bind, int port) {
  if (th_.joinable()) return false;
  server_.reset(new httplib::Server());
  server_->Get("/metrics", [](const httplib::Request&, httplib::Response& res){
    std::ostringstream oss; M().to_prometheus(oss);
    res.set_content(oss.str(), "text/plain");
  });
  server_->Get("/metrics.json", [](const httplib::Request&, httplib::Response& res){
    std::ostringstream oss; M().to_json(oss);
    res.set_content(oss.str(), "application/json");
  });
  int actual = 0;
  if (port==0) {
    actual = server_->bind_to_any_port(bind.c_str());
    if(actual<=0) return false;
  } else {
    if(!server_->bind_to_port(bind.c_str(), port)) return false;
    actual = port;
  }
  port_ = actual;
  th_ = std::thread([this](){ server_->listen_after_bind(); });
  server_->wait_until_ready();
  return true;
}

void MetricsEndpoint::stop() {
  if (server_) server_->stop();
  if (th_.joinable()) th_.join();
}

} // namespace obs

