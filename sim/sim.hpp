#pragma once
#include <cstdint>
#include <vector>
#include <functional>

struct SimState {
  bool endstop_open = false;
  bool endstop_closed = false;
  bool hopper_sig = false;
  int hopper_pulses = 0;
  int32_t scale_raw = 0;
  int32_t scale_base = 0;
};

struct SimEvent {
  uint64_t ms;
  std::function<void(SimState&)> fn;
};

class Simulator {
public:
  void add_event(uint64_t ms, std::function<void(SimState&)> fn);
  void run();
  const SimState& state() const { return st_; }
private:
  SimState st_;
  std::vector<SimEvent> events_;
};

Simulator make_default_sim();
