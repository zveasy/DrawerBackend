#pragma once

#include "../drivers/hopper_parallel.hpp"
#include "../util/persist.hpp"
#include <string>

struct DispenseConfig {
  int max_retries = 2;      // attempt count beyond the first run
  int jam_ms = 600;         // if pulses don't increase for this long -> jam
  int settle_ms = 120;      // grace after motor start
  int max_ms_per_coin = 300;
  int hard_timeout_ms = 5000;
  int poll_us = 200;        // polling interval for hopper sensor
};

struct DispenseResult {
  bool ok = false;
  int requested = 0;
  int dispensed = 0;
  int pulses = 0;
  int retries = 0;
  std::string reason = "ABORT"; // OK, JAM, TIMEOUT, PARTIAL, ABORT
  int elapsed_ms = 0;
};

class DispenseController {
public:
  DispenseController(HopperParallel& hopper, const DispenseConfig& cfg,
                     const std::string& calib_path = "data/calibration.txt");

  void loadCalibration();
  void saveCalibration(int pulses_per_coin);
  DispenseResult dispenseCoins(int coins);

private:
  HopperParallel& hopper_;
  DispenseConfig cfg_;
  std::string path_;
};

