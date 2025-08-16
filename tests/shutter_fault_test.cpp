#include <gtest/gtest.h>
#include "drivers/stepper.hpp"
#include "app/shutter_fsm.hpp"
#include "hal/gpio.hpp"

TEST(ShutterFSM, HomingTimeoutFault) {
  auto chip = hal::make_mock_chip();
  Stepper step(*chip,1,2,3,4,5,10,1,10); // small limits
  ShutterFSM fsm(step,5,10);
  fsm.cmdHome();
  for(int i=0;i<150 && fsm.state()!=ShutterState::FAULT;i++) {
    fsm.tick();
  }
  EXPECT_EQ(fsm.state(), ShutterState::FAULT);
}

TEST(ShutterFSM, SoftLimitFault) {
  auto chip = hal::make_mock_chip();
  Stepper step(*chip,1,2,3,4,5,40,1,80);
  ShutterFSM fsm(step,5,80);
  auto closed = chip->request_line(5, hal::Direction::In);
  fsm.cmdHome();
  closed->write(true);
  for(int i=0;i<5 && fsm.state()!=ShutterState::CLOSED;i++) fsm.tick();
  ASSERT_EQ(fsm.state(), ShutterState::CLOSED);
  fsm.cmdOpenMm(200);
  EXPECT_EQ(fsm.state(), ShutterState::FAULT);
}
