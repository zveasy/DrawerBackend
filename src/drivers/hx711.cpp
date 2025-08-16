#include "hx711.hpp"
#include "hal/time.hpp"
#include <stdexcept>

HX711::HX711(hal::Chip& chip, int dt_pin, int sck_pin, int gain)
: gain_(gain)
{
  dt_  = chip.request_line(dt_pin,  hal::Direction::In);
  sck_ = chip.request_line(sck_pin, hal::Direction::Out);
  sck_->write(false);
}

long HX711::read_raw() {
  // wait for data ready (DT goes low)
  int guard = 0;
  while (dt_->read()) { hal::sleep_us(10); if(++guard > 500000) throw std::runtime_error("HX711 data not ready"); }
  long val = 0;
  for (int i=0; i<24; ++i) {
    sck_->write(true);
    hal::sleep_us(1);
    val = (val << 1) | (dt_->read() ? 1 : 0);
    sck_->write(false);
    hal::sleep_us(1);
  }
  // set gain|channel by extra pulses
  int pulses = (gain_==128 ? 1 : (gain_==64 ? 3 : 2));
  for(int i=0;i<pulses;i++){ sck_->write(true); hal::sleep_us(1); sck_->write(false); hal::sleep_us(1); }
  if (val & 0x800000) val |= ~0xFFFFFF; // sign extend
  return val;
}

long HX711::read_average(int samples) {
  long sum = 0;
  for(int i=0;i<samples;i++) sum += read_raw();
  return sum / samples;
}

