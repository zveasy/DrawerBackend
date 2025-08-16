#pragma once
#include "../hal/gpio.hpp"
#include <cstdint>
#include <chrono>
#include <string>

class Stepper {
public:
  // DRV8825 style: step, dir, enable (active-low), two endstops (NC or NO; we read truth directly)
  Stepper(hal::Chip& chip, int pin_step, int pin_dir, int pin_enable,
          int pin_limit_open, int pin_limit_closed,
          int steps_per_mm, int pulse_us=400);

  void enable(bool en);
  void set_dir(bool open_direction);
  void step(int microsteps = 1); // blocking pulse
  bool at_open() const;
  bool at_closed() const;

  // Basic moves
  bool home_closed(int max_mm);   // move toward CLOSED until limit; returns success
  bool open_mm(int mm);           // move toward OPEN measured by steps
  bool close_mm(int mm);

private:
  hal::Line* step_{};
  hal::Line* dir_{};
  hal::Line* en_{};
  hal::Line* lim_open_{};
  hal::Line* lim_closed_{};
  int steps_per_mm_{};
  int pulse_us_{};
};

