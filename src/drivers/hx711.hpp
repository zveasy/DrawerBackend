#pragma once
#include "../hal/gpio.hpp"
#include <cstdint>

class HX711 {
public:
  // dt_pin: data, sck_pin: clock; gain=128 channel A default
  HX711(hal::Chip& chip, int dt_pin, int sck_pin, int gain = 128);
  long read_raw();
  long read_average(int times = 8);

private:
  hal::Line* dt_{};
  hal::Line* sck_{};
  int gain_{};
};

