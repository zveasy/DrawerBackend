#include "shutter_adapter.hpp"
#include <thread>

ShutterAdapter::ShutterAdapter(ShutterFSM& fsm) : fsm_(fsm) {}

bool ShutterAdapter::waitFor(ShutterState target, std::string* reason) {
  for (int i = 0; i < 10000; ++i) {
    fsm_.tick();
    ShutterState st = fsm_.state();
    if (st == target) return true;
    if (st == ShutterState::FAULT) {
      if (reason) *reason = "FAULT";
      return false;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
  if (reason) *reason = "TIMEOUT";
  return false;
}

bool ShutterAdapter::home(int /*max_mm*/, std::string* reason) {
  fsm_.cmdHome();
  return waitFor(ShutterState::CLOSED, reason);
}

bool ShutterAdapter::open_mm(int mm, std::string* reason) {
  fsm_.cmdOpenMm(mm);
  return waitFor(ShutterState::OPEN, reason);
}

bool ShutterAdapter::close_mm(int mm, std::string* reason) {
  fsm_.cmdCloseMm(mm);
  return waitFor(ShutterState::CLOSED, reason);
}
