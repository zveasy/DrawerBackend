#pragma once

#include "hal/gpio.hpp"
#include "hal/time.hpp"
#include <cstdint>

// Driver for a simple parallel coin hopper. The motor is controlled by a
// digital output and an optic sensor produces pulses while coins exit the
// hopper.
class HopperParallel {
public:
  HopperParallel(hal::Chip& chip,
                 int motor_en_pin,
                 int pulse_in_pin,
                 bool motor_active_high = true,
                 int pulses_per_coin = 1,
                 int min_edge_interval_us = 2000);

  void start();
  void stop();
  void resetCounters();
  void setPulsesPerCoin(int p);
  int  getPulsesPerCoin() const { return pulses_per_coin_; }
  int  pulseCount() const { return pulses_; }
  int  coinCount() const { return pulses_per_coin_ > 0 ? pulses_ / pulses_per_coin_ : 0; }

  // Poll once: read current input, perform edge detection and update counters.
  void pollOnce();

private:
  hal::Line* motor_{};
  hal::Line* pulse_{};
  bool motor_active_high_{};
  int pulses_per_coin_{};
  int min_interval_us_{};
  int pulses_{0};
  uint64_t last_edge_ns_{0};
  bool last_level_{false};
};

