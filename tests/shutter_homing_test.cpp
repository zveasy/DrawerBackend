#include <gtest/gtest.h>
#include "drivers/stepper.hpp"
#include "app/shutter_fsm.hpp"
#include "hal/gpio.hpp"

TEST(ShutterFSM, HomingDebounce) {
  auto chip = hal::make_mock_chip();
  // arbitrary pin numbers
  Stepper step(*chip, 1,2,3,4,5,40,1,80);
  ShutterFSM fsm(step, 5, 80);
  auto closed = chip->request_line(5, hal::Direction::In);
  fsm.cmdHome();
  for(int i=0;i<20 && fsm.state()!=ShutterState::CLOSED && fsm.state()!=ShutterState::FAULT;i++) {
    if(i==2) closed->write(true); // short flicker
    if(i==3) closed->write(false);
    if(i==5) closed->write(true); // stable
    fsm.tick();
  }
  EXPECT_EQ(fsm.state(), ShutterState::CLOSED);
}
