#include "hopper_parallel.hpp"

HopperParallel::HopperParallel(hal::Chip& chip,
                               int motor_en_pin,
                               int pulse_in_pin,
                               bool motor_active_high,
                               int pulses_per_coin,
                               int min_edge_interval_us)
    : motor_active_high_(motor_active_high),
      pulses_per_coin_(pulses_per_coin),
      min_interval_us_(min_edge_interval_us) {
  motor_ = chip.request_line(motor_en_pin, hal::Direction::Out);
  pulse_ = chip.request_line(pulse_in_pin, hal::Direction::In);
  stop();
  last_level_ = pulse_->read();
}

void HopperParallel::start() { motor_->write(motor_active_high_); }

void HopperParallel::stop() { motor_->write(!motor_active_high_); }

void HopperParallel::resetCounters() {
  pulses_ = 0;
  last_edge_ns_ = 0;
  last_level_ = pulse_->read();
}

void HopperParallel::setPulsesPerCoin(int p) { pulses_per_coin_ = p; }

void HopperParallel::pollOnce() {
  bool level = pulse_->read();
  uint64_t now = hal::now_ns();
  if (level && !last_level_) {
    if (now - last_edge_ns_ >= static_cast<uint64_t>(min_interval_us_) * 1000) {
      ++pulses_;
      last_edge_ns_ = now;
    }
  }
  last_level_ = level;
}

