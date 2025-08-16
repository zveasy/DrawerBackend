#include "sim.hpp"
#include <algorithm>

void Simulator::add_event(uint64_t ms, std::function<void(SimState&)> fn) {
  events_.push_back({ms, std::move(fn)});
}

void Simulator::run() {
  std::sort(events_.begin(), events_.end(),
            [](const SimEvent& a, const SimEvent& b){ return a.ms < b.ms; });
  for (auto& e : events_) {
    if (e.fn) e.fn(st_);
  }
}

Simulator make_default_sim() {
  Simulator sim;
  sim.add_event(0, [](SimState& s){ s.endstop_closed = true; s.scale_raw = 0; s.scale_base = 0; });
  sim.add_event(100, [](SimState& s){ s.hopper_sig = !s.hopper_sig; s.hopper_pulses++; });
  sim.add_event(200, [](SimState& s){ s.hopper_sig = !s.hopper_sig; s.hopper_pulses++; });
  sim.add_event(300, [](SimState& s){ s.hopper_sig = !s.hopper_sig; s.hopper_pulses++; });
  sim.add_event(400, [](SimState& s){ s.scale_raw = s.scale_base + 1000; });
  return sim;
}
