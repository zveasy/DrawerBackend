#pragma once
#include "hal/gpio.hpp"
#include <cstdint>

class Stepper {
public:
  Stepper(hal::Chip& chip,
          int pin_step,
          int pin_dir,
          int pin_enable,
          int pin_limit_open,
          int pin_limit_closed,
          int steps_per_mm,
          int pulse_us,
          int max_mm);

  void enable(bool en);
  void setDir(bool open_dir);
  void step(int microsteps = 1);
  bool atOpen() const;
  bool atClosed() const;

  bool homeClosed(int max_mm);
  bool moveOpenMm(int mm);
  bool moveCloseMm(int mm);

  int steps_per_mm() const { return steps_per_mm_; }
  int max_mm() const { return max_mm_; }

private:
  hal::Line* step_{};
  hal::Line* dir_{};
  hal::Line* en_{};
  hal::Line* lim_open_{};
  hal::Line* lim_closed_{};
  int steps_per_mm_{};
  int pulse_us_{};
  int max_mm_{};
};
