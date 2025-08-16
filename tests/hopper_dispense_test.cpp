#include <gtest/gtest.h>
#include <thread>

#include "../hal/gpio.hpp"
#include "../src/drivers/hopper_parallel.hpp"
#include "../src/app/dispense_ctrl.hpp"

using namespace std::chrono_literals;

TEST(HopperDispense, DispenseThreeCoins) {
  auto chip = hal::make_mock_chip();
  HopperParallel hopper(*chip, 1, 2);
  DispenseConfig cfg;
  cfg.settle_ms = 0;
  DispenseController ctrl(hopper, cfg, "/tmp/calib_dispense.txt");

  std::thread t([&] {
    // Wait for motor to turn on
    while (!hal::mock::get_output(1)) std::this_thread::sleep_for(1ms);
    hal::sleep_us(5000);
    for (int i = 0; i < 3; ++i) {
      hal::mock::set_input(2, true);
      hal::sleep_us(3000);
      hal::mock::set_input(2, false);
      hal::sleep_us(3000);
    }
  });

  auto res = ctrl.dispenseCoins(3);
  t.join();

  EXPECT_TRUE(res.ok);
  EXPECT_EQ(res.dispensed, 3);
  EXPECT_EQ(res.reason, "OK");
  EXPECT_LT(res.elapsed_ms, 1000);
}

