#include "dispense_ctrl.hpp"

#include <chrono>
#include "hal/time.hpp"

DispenseController::DispenseController(HopperParallel& hopper,
                                       const DispenseConfig& cfg,
                                       const std::string& calib_path)
    : hopper_(hopper), cfg_(cfg), path_(calib_path) {}

void DispenseController::loadCalibration() {
  std::map<std::string, std::string> kv;
  if (persist::load_kv(path_, kv)) {
    auto it = kv.find("pulses_per_coin");
    if (it != kv.end()) {
      hopper_.setPulsesPerCoin(std::stoi(it->second));
    }
  }
}

void DispenseController::saveCalibration(int p) {
  std::map<std::string, std::string> kv{{"pulses_per_coin", std::to_string(p)}};
  persist::save_kv(path_, kv);
}

DispenseResult DispenseController::dispenseCoins(int coins) {
  using namespace std::chrono;
  DispenseResult res;
  res.requested = coins;

  auto start = steady_clock::now();
  int budget_ms = std::max(cfg_.hard_timeout_ms, coins * cfg_.max_ms_per_coin + 500);

  int attempt = 0;
  for (attempt = 0; attempt <= cfg_.max_retries; ++attempt) {
    hopper_.resetCounters();
    hopper_.start();
    hal::sleep_us(cfg_.settle_ms * 1000);
    int last_pulses = hopper_.pulseCount();
    auto last_pulse_time = steady_clock::now();
    while (true) {
      hopper_.pollOnce();
      auto now = steady_clock::now();
      int pc = hopper_.pulseCount();
      if (pc != last_pulses) {
        last_pulses = pc;
        last_pulse_time = now;
      }
      if (hopper_.coinCount() >= coins) {
        hopper_.stop();
        res.ok = true;
        res.reason = "OK";
        goto done;
      }
      if (duration_cast<milliseconds>(now - last_pulse_time).count() >= cfg_.jam_ms) {
        hopper_.stop();
        res.ok = false;
        res.reason = "JAM";
        break; // retry if allowed
      }
      if (duration_cast<milliseconds>(now - start).count() >= budget_ms) {
        hopper_.stop();
        res.ok = false;
        res.reason = hopper_.coinCount() > 0 ? "PARTIAL" : "TIMEOUT";
        goto done;
      }
      hal::sleep_us(1000);
    }
    if (attempt < cfg_.max_retries) {
      hal::sleep_us(100000); // backoff
      continue;
    } else {
      break; // out of retries
    }
  }

done:
  res.dispensed = hopper_.coinCount();
  res.pulses = hopper_.pulseCount();
  res.retries = attempt;
  res.elapsed_ms = duration_cast<milliseconds>(steady_clock::now() - start).count();
  if (!res.ok && res.reason == "JAM" && res.dispensed > 0 && res.dispensed < res.requested) {
    res.reason = "PARTIAL"; // jam after some coins
  }
  return res;
}

