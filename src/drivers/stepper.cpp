#include "stepper.hpp"
#include <iostream>

Stepper::Stepper(hal::Chip& chip, int pstep, int pdir, int penable, int plim_open, int plim_closed,
                 int steps_per_mm, int pulse_us)
: steps_per_mm_(steps_per_mm), pulse_us_(pulse_us)
{
  step_ = chip.request_line(pstep, hal::Direction::Out);
  dir_  = chip.request_line(pdir,  hal::Direction::Out);
  en_   = chip.request_line(penable, hal::Direction::Out);
  lim_open_   = chip.request_line(plim_open,   hal::Direction::In);
  lim_closed_ = chip.request_line(plim_closed, hal::Direction::In);
  enable(false); // disable driver initially (DRV8825 EN high disables)
}

void Stepper::enable(bool en) { en_->write(!en); } // active-low enable
void Stepper::set_dir(bool open_dir) { dir_->write(open_dir); }
void Stepper::step(int microsteps) {
  for(int i=0;i<microsteps;i++){
    step_->write(true);
    hal::busy_wait_us(pulse_us_);
    step_->write(false);
    hal::busy_wait_us(pulse_us_);
  }
}
bool Stepper::at_open()   const { return lim_open_->read(); }
bool Stepper::at_closed() const { return lim_closed_->read(); }

bool Stepper::home_closed(int max_mm) {
  enable(true);
  set_dir(false); // assume false==toward closed
  int max_steps = max_mm * steps_per_mm_;
  for(int i=0;i<max_steps;i++){
    if (at_closed()) { std::cerr<<"[STEP] Homed CLOSED\n"; enable(false); return true; }
    const_cast<Stepper*>(this)->step();
  }
  std::cerr<<"[STEP] Home CLOSED timeout\n";
  enable(false);
  return false;
}

bool Stepper::open_mm(int mm){
  enable(true);
  set_dir(true);
  int steps = mm * steps_per_mm_;
  for(int i=0;i<steps;i++){
    if (at_open()) { std::cerr<<"[STEP] Hit OPEN limit early at "<<i<<" steps\n"; break; }
    step();
  }
  enable(false);
  return true;
}

bool Stepper::close_mm(int mm){
  enable(true);
  set_dir(false);
  int steps = mm * steps_per_mm_;
  for(int i=0;i<steps;i++){
    if (at_closed()) { std::cerr<<"[STEP] Hit CLOSED limit early at "<<i<<" steps\n"; break; }
    step();
  }
  enable(false);
  return true;
}

