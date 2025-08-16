#pragma once
#include "drivers/stepper.hpp"
#include <string>

enum class ShutterState { INIT, HOMING, CLOSED, OPENING, OPEN, CLOSING, FAULT };

class ShutterFSM {
public:
  ShutterFSM(Stepper& stepper, int debounce_ms, int max_mm);

  void cmdHome();
  void cmdOpenMm(int mm);
  void cmdCloseMm(int mm);
  void tick();
  ShutterState state() const { return state_; }

private:
  Stepper& stepper_;
  ShutterState state_{ShutterState::INIT};
  int debounce_reads_;
  int debounce_interval_us_;
  int max_mm_;
  int target_steps_{};
  int steps_done_{};
  int deb_open_{};
  int deb_closed_{};

  void setState(ShutterState to, const std::string& reason);
  bool debounce(bool level, int& counter);
  static const char* stateName(ShutterState s);
};
