#include <gtest/gtest.h>
#include <thread>

#include "../hal/gpio.hpp"
#include "../src/drivers/hopper_parallel.hpp"
#include "../src/app/dispense_ctrl.hpp"

using namespace std::chrono_literals;

TEST(HopperNoise, DoublePulseIgnored) {
  auto chip = hal::make_mock_chip();
  HopperParallel hopper(*chip, 7, 8, true, 1, 2000);
  DispenseConfig cfg;
  cfg.settle_ms = 0;
  DispenseController ctrl(hopper, cfg, "/tmp/calib_noise.txt");

  std::thread t([&] {
    while (!hal::mock::get_output(7)) std::this_thread::sleep_for(1ms);
    hal::sleep_us(5000);

    // First coin with bounce (double pulse <2ms)
    hal::mock::set_input(8, true);
    hal::sleep_us(500);
    hal::mock::set_input(8, false);
    hal::sleep_us(500);
    hal::mock::set_input(8, true); // bounce
    hal::sleep_us(500);
    hal::mock::set_input(8, false);

    hal::sleep_us(3000);

    // Second coin clean
    hal::mock::set_input(8, true);
    hal::sleep_us(3000);
    hal::mock::set_input(8, false);
  });

  auto res = ctrl.dispenseCoins(2);
  t.join();

  EXPECT_TRUE(res.ok);
  EXPECT_EQ(res.dispensed, 2);
  EXPECT_EQ(res.pulses, 2);
}

