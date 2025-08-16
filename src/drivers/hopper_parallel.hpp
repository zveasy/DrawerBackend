#pragma once
#include "../hal/gpio.hpp"
#include <cstdint>
#include <chrono>

class HopperParallel {
public:
  // motor_en: output to MOSFET/relay (true=run), pulse_in: optic sensor pulses per coin (usually >1; we use edges)
  HopperParallel(hal::Chip& chip, int motor_en_pin, int pulse_in_pin, int pulses_per_coin = 1);

  // Dispense N coins with max safety timeout (ms). Returns true if success.
  bool dispense(int coins, int max_ms = 5000);

private:
  hal::Line* en_{};
  hal::Line* pulse_{};
  int pulses_per_coin_{};
};

