#include "drivers/hopper_parallel.hpp"
#include "drivers/stepper.hpp"
#include "app/shutter_fsm.hpp"
#include "app/dispense_ctrl.hpp"
#include "hal/gpio.hpp"
#include "util/log.hpp"

#include <cstdlib>
#include <iostream>
#include <string>

// --- PIN MAP ---
static constexpr int PIN_STEP = 23;
static constexpr int PIN_DIR  = 24;
static constexpr int PIN_EN   = 25;
static constexpr int PIN_OPEN = 26;
static constexpr int PIN_CLOSED = 27;

static constexpr int PIN_HOPPER_EN = 5;
static constexpr int PIN_HOPPER_P  = 6;

int main(int argc, char** argv) {
  bool demo_shutter = false;
  int dispense_n = -1;
  for (int i = 1; i < argc; ++i) {
    std::string arg(argv[i]);
    if (arg == "--json") {
      setenv("LOG_JSON", "1", 1);
    } else if (arg == "--demo-shutter") {
      demo_shutter = true;
    } else if (arg == "--dispense" && i + 1 < argc) {
      dispense_n = std::stoi(argv[++i]);
    }
  }

  try {
    auto chip = hal::make_chip();

    if (demo_shutter) {
      Stepper step(*chip, PIN_STEP, PIN_DIR, PIN_EN, PIN_OPEN, PIN_CLOSED,
                   /*steps_per_mm=*/40, /*pulse_us=*/400, /*rpm=*/80);
      ShutterFSM fsm(step, 5, 80);
      fsm.cmdHome();
      while (fsm.state() != ShutterState::CLOSED &&
             fsm.state() != ShutterState::FAULT) {
        fsm.tick();
      }
      return fsm.state() == ShutterState::CLOSED ? 0 : 1;
    }

    if (dispense_n >= 0) {
      HopperParallel hopper(*chip, PIN_HOPPER_EN, PIN_HOPPER_P);
      DispenseConfig cfg;
      DispenseController ctrl(hopper, cfg);
      ctrl.loadCalibration();
      auto res = ctrl.dispenseCoins(dispense_n);
      LOG_INFO("dispense",
               {{"requested", std::to_string(res.requested)},
                {"dispensed", std::to_string(res.dispensed)},
                {"pulses", std::to_string(res.pulses)},
                {"retries", std::to_string(res.retries)},
                {"reason", res.reason},
                {"ms", std::to_string(res.elapsed_ms)}});
      if (res.ok) {
        std::cout << res.dispensed << " coins dispensed" << std::endl;
        return 0;
      } else {
        std::cout << "Dispense failed: " << res.reason << std::endl;
        return 1;
      }
    }

    std::cout << "No action specified. Use --demo-shutter or --dispense N" << std::endl;
    return 0;
  } catch (const std::exception& e) {
    LOG_ERROR("main", {{"err", e.what()}});
    std::cerr << e.what() << std::endl;
    return 1;
  }
}

