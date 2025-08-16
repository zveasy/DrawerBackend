#include "hopper_parallel.hpp"
#include <iostream>

HopperParallel::HopperParallel(hal::Chip& chip, int motor_en_pin, int pulse_in_pin, int pulses_per_coin)
: pulses_per_coin_(pulses_per_coin)
{
  en_    = chip.request_line(motor_en_pin, hal::Direction::Out);
  pulse_ = chip.request_line(pulse_in_pin, hal::Direction::In);
  en_->write(false); // motor off
}

bool HopperParallel::dispense(int coins, int max_ms) {
  const int target_pulses = coins * pulses_per_coin_;
  int pulses = 0;
  bool last = pulse_->read();
  en_->write(true); // start motor
  auto start = std::chrono::steady_clock::now();
  while (pulses < target_pulses) {
    bool cur = false;
    try { cur = pulse_->read(); } catch(...) { en_->write(false); throw; }
    if (cur && !last) pulses++; // rising edge
    last = cur;
    hal::sleep_us(500); // simple poll; replace with interrupts/epoll later

    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - start).count();
    if (ms > max_ms) {
      en_->write(false);
      std::cerr << "[HOPPER] Timeout after " << ms << " ms ("<<pulses<<"/"<<target_pulses<<" pulses)\n";
      return false;
    }
  }
  en_->write(false);
  std::cerr << "[HOPPER] Dispensed " << coins << " coin(s)\n";
  return true;
}

