#include "dispense_adapter.hpp"

DispenseAdapter::DispenseAdapter(DispenseController& ctrl) : ctrl_(ctrl) {}

DispenseStats DispenseAdapter::dispenseCoins(int coins) {
  auto r = ctrl_.dispenseCoins(coins);
  DispenseStats s;
  s.ok = r.ok;
  s.requested = r.requested;
  s.dispensed = r.dispensed;
  s.pulses = r.pulses;
  s.retries = r.retries;
  s.reason = r.reason;
  s.elapsed_ms = r.elapsed_ms;
  return s;
}
