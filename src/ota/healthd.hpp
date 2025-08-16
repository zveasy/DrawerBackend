#pragma once
#include "ota/backend.hpp"
#include "config/config.hpp"
#include <string>

namespace ota {

class Healthd {
public:
  Healthd(const cfg::Config& cfg, IOtaBackend& backend);
  OtaResult run_once(const std::string& base_url);
private:
  cfg::Config cfg_;
  IOtaBackend& backend_;
};

} // namespace ota

