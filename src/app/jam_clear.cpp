#include "app/jam_clear.hpp"
#include <thread>

namespace jam_clear {

bool shutter(Stepper& stp, const cfg::Service& cfg, safety::FaultManager& fm) {
  (void)stp; (void)cfg; (void)fm; // stub
  fm.clear(safety::FaultCode::JAM_SHUTTER);
  return true;
}

bool hopper(IDispenser& d, const cfg::Service& cfg, safety::FaultManager& fm) {
  (void)cfg;
  d.dispenseCoins(0); // treat as nudge
  fm.clear(safety::FaultCode::JAM_HOPPER);
  return true;
}

} // namespace jam_clear

