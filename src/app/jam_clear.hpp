#pragma once
#include "drivers/stepper.hpp"
#include "app/idispenser.hpp"
#include "config/config.hpp"
#include "safety/faults.hpp"

namespace jam_clear {

bool shutter(Stepper& stp, const cfg::Service& cfg, safety::FaultManager& fm);
bool hopper(IDispenser& d, const cfg::Service& cfg, safety::FaultManager& fm);

} // namespace jam_clear

