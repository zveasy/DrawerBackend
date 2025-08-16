#include "app/shutter_fsm.hpp"
#include "util/log.hpp"
#include "hal/time.hpp"
#include <algorithm>

ShutterFSM::ShutterFSM(Stepper& stepper, int debounce_ms, int max_mm)
  : stepper_(stepper),
    debounce_reads_(3),
    debounce_interval_us_(debounce_ms*1000/3),
    max_mm_(max_mm) {}

void ShutterFSM::setState(ShutterState to, const std::string& reason) {
  if (state_ != to) {
    LOG_INFO("shutter_state", {{"from", stateName(state_)}, {"to", stateName(to)}, {"reason", reason}});
    state_ = to;
  }
}

const char* ShutterFSM::stateName(ShutterState s) {
  switch(s) {
    case ShutterState::INIT: return "INIT";
    case ShutterState::HOMING: return "HOMING";
    case ShutterState::CLOSED: return "CLOSED";
    case ShutterState::OPENING: return "OPENING";
    case ShutterState::OPEN: return "OPEN";
    case ShutterState::CLOSING: return "CLOSING";
    case ShutterState::FAULT: return "FAULT";
  }
  return "?";
}

bool ShutterFSM::debounce(bool level, int& counter) {
  if (level) {
    counter++;
    if (counter >= debounce_reads_) return true;
  } else {
    counter = 0;
  }
  return false;
}

void ShutterFSM::cmdHome() {
  steps_done_ = 0;
  target_steps_ = max_mm_ * stepper_.steps_per_mm();
  deb_closed_ = 0;
  stepper_.enable(true);
  stepper_.setDir(false);
  setState(ShutterState::HOMING, "cmd_home");
}

void ShutterFSM::cmdOpenMm(int mm) {
  if (mm > max_mm_) { setState(ShutterState::FAULT, "soft_limit"); return; }
  steps_done_ = 0;
  target_steps_ = mm * stepper_.steps_per_mm();
  deb_open_ = 0;
  stepper_.enable(true);
  stepper_.setDir(true);
  setState(ShutterState::OPENING, "cmd_open");
}

void ShutterFSM::cmdCloseMm(int mm) {
  if (mm > max_mm_) { setState(ShutterState::FAULT, "soft_limit"); return; }
  steps_done_ = 0;
  target_steps_ = mm * stepper_.steps_per_mm();
  deb_closed_ = 0;
  stepper_.enable(true);
  stepper_.setDir(false);
  setState(ShutterState::CLOSING, "cmd_close");
}

void ShutterFSM::tick() {
  hal::sleep_us(debounce_interval_us_);
  switch(state_) {
    case ShutterState::HOMING: {
      stepper_.step();
      steps_done_++;
      if (debounce(stepper_.atClosed(), deb_closed_)) {
        stepper_.enable(false);
        setState(ShutterState::CLOSED, "home_ok");
      } else if (steps_done_ >= target_steps_) {
        stepper_.enable(false);
        setState(ShutterState::FAULT, "home_timeout");
      }
      break;
    }
    case ShutterState::OPENING: {
      stepper_.step();
      steps_done_++;
      if (debounce(stepper_.atOpen(), deb_open_)) {
        stepper_.enable(false);
        setState(ShutterState::OPEN, "hit_open");
      } else if (steps_done_ >= target_steps_) {
        stepper_.enable(false);
        setState(ShutterState::OPEN, "target");
      } else if (steps_done_ >= max_mm_ * stepper_.steps_per_mm()) {
        stepper_.enable(false);
        setState(ShutterState::FAULT, "soft_limit");
      }
      break;
    }
    case ShutterState::CLOSING: {
      stepper_.step();
      steps_done_++;
      if (debounce(stepper_.atClosed(), deb_closed_)) {
        stepper_.enable(false);
        setState(ShutterState::CLOSED, "hit_closed");
      } else if (steps_done_ >= target_steps_) {
        stepper_.enable(false);
        setState(ShutterState::CLOSED, "target");
      } else if (steps_done_ >= max_mm_ * stepper_.steps_per_mm()) {
        stepper_.enable(false);
        setState(ShutterState::FAULT, "soft_limit");
      }
      break;
    }
    default:
      break;
  }
}
