#pragma once
#include "eol/eol_spec.hpp"
#include "app/ishutter.hpp"
#include "app/idispenser.hpp"
#include "drivers/iscale.hpp"
#include "config/config.hpp"
#include <string>
#include <vector>

namespace eol {

class Runner {
public:
  Runner(IShutter& sh, IDispenser& disp, IScale* sc, std::string telemetry_path);
  EolResult run_once(const cfg::Config& cfg);
  static bool stamp_pass_fail(const EolResult& r);
  const std::vector<EolStepResult>& status() const { return current_steps_; }
private:
  IShutter& shutter_;
  IDispenser& dispenser_;
  IScale* scale_;
  std::string telemetry_path_;
  std::vector<EolStepResult> current_steps_;
};

} // namespace eol
