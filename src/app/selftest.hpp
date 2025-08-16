#pragma once
#include <string>
#include "config/config.hpp"
#include "ishutter.hpp"
#include "idispenser.hpp"
#include "../drivers/iscale.hpp"

namespace selftest {

struct Result {
  bool ok;
  std::string json; // full health JSON
};

Result run(const cfg::Config& c);
Result run(IShutter& sh, IDispenser& disp, IScale* scale, const cfg::Config& c);

} // namespace selftest
