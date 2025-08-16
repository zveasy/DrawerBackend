#pragma once
#include "ota/backend.hpp"
#include "config/config.hpp"
#include <string>

namespace ota {

class Agent {
public:
  Agent(const cfg::Config& cfg, IOtaBackend& backend);
  OtaResult run_once();

  static int hash_device(const std::string& id);
  static bool allow(int h, int percent);
private:
  cfg::Config cfg_;
  IOtaBackend& backend_;
};

} // namespace ota

