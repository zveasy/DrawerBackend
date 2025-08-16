#include "stepper.hpp"

Stepper::Stepper(hal::Chip& chip,
                 int pin_step,
                 int pin_dir,
                 int pin_enable,
                 int pin_limit_open,
                 int pin_limit_closed,
                 int steps_per_mm,
                 int pulse_us,
                 int max_mm)
    : steps_per_mm_(steps_per_mm),
      pulse_us_(pulse_us),
      max_mm_(max_mm) {
  step_ = chip.request_line(pin_step, hal::Direction::Out);
  dir_ = chip.request_line(pin_dir, hal::Direction::Out);
  en_ = chip.request_line(pin_enable, hal::Direction::Out);
  lim_open_ = chip.request_line(pin_limit_open, hal::Direction::In);
  lim_closed_ = chip.request_line(pin_limit_closed, hal::Direction::In);
  enable(false);
}

void Stepper::enable(bool en) { en_->write(!en); }

void Stepper::setDir(bool open_dir) { dir_->write(open_dir); }

void Stepper::step(int microsteps) {
  for (int i = 0; i < microsteps; ++i) {
    step_->write(true);
    hal::sleep_us(pulse_us_);
    step_->write(false);
    hal::sleep_us(pulse_us_);
  }
}

bool Stepper::atOpen() const { return lim_open_->read(); }
bool Stepper::atClosed() const { return lim_closed_->read(); }

bool Stepper::homeClosed(int max_mm) {
  setDir(false);
  enable(true);
  int max_steps = max_mm * steps_per_mm_;
  for (int i = 0; i < max_steps; ++i) {
    if (atClosed()) { enable(false); return true; }
    step();
  }
  enable(false);
  return false;
}

bool Stepper::moveOpenMm(int mm) {
  setDir(true);
  enable(true);
  int steps = mm * steps_per_mm_;
  for (int i = 0; i < steps; ++i) step();
  enable(false);
  return true;
}

bool Stepper::moveCloseMm(int mm) {
  setDir(false);
  enable(true);
  int steps = mm * steps_per_mm_;
  for (int i = 0; i < steps; ++i) step();
  enable(false);
  return true;
}

