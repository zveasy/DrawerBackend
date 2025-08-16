#include <gtest/gtest.h>

#include "../hal/gpio.hpp"
#include "../src/drivers/hopper_parallel.hpp"
#include "../src/app/dispense_ctrl.hpp"

TEST(HopperJam, NoPulses) {
  auto chip = hal::make_mock_chip();
  HopperParallel hopper(*chip, 3, 4);
  DispenseConfig cfg;
  cfg.jam_ms = 100;      // speed up test
  cfg.max_retries = 1;   // only one retry
  cfg.settle_ms = 0;
  DispenseController ctrl(hopper, cfg, "/tmp/calib_jam.txt");

  auto res = ctrl.dispenseCoins(1);
  EXPECT_FALSE(res.ok);
  EXPECT_EQ(res.dispensed, 0);
  EXPECT_NE(res.reason, "OK");
}

