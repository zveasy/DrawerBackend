#pragma once
#include <httplib.h>
#include <atomic>
#include "eol/eol_runner.hpp"
#include "config/config.hpp"

namespace server {

class EolEndpoint {
public:
  EolEndpoint(eol::Runner& r, const cfg::Config& c);
  void register_routes(httplib::Server& svr);
private:
  eol::Runner& runner_;
  const cfg::Config& cfg_;
  std::atomic<bool> busy_{false};
  eol::EolResult last_{};
};

} // namespace server
