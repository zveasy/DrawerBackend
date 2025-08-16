#pragma once
#include <string>
#include "config/config.hpp"
#include "safety/faults.hpp"
#include "util/event_log.hpp"
#include "app/ishutter.hpp"
#include "app/idispenser.hpp"
#include "drivers/stepper.hpp"
#include "app/jam_clear.hpp"

class ServiceMode {
public:
  ServiceMode(const cfg::Service& cfg, safety::FaultManager& fm, eventlog::Logger& elog);
  bool enter(const std::string& pin, const std::string& user="tech");
  void exit();
  bool active() const { return active_; }
  bool cmd_open_shutter(IShutter& sh, int mm);
  bool cmd_close_shutter(IShutter& sh, int mm);
  bool cmd_dispense(IDispenser& d, int coins);
  bool run_jam_clear_shutter(Stepper& stp, const cfg::Service& s);
  bool run_jam_clear_hopper(IDispenser& d, const cfg::Service& s);
private:
  void log(const std::map<std::string,std::string>& kv);
  bool guard();
  cfg::Service cfg_;
  safety::FaultManager& fm_;
  eventlog::Logger& elog_;
  bool active_{false};
  std::string user_;
};

