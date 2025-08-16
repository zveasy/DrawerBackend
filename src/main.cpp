
#include "sim/sim.hpp"
#include "hal/gpio.hpp"
#include "util/log.hpp"
#include "drivers/stepper.hpp"
#include "app/shutter_fsm.hpp"
#include <iostream>
#include <cstdlib>

static constexpr int PIN_STEP = 23;
static constexpr int PIN_DIR  = 24;
static constexpr int PIN_EN   = 25;
static constexpr int PIN_OPEN = 26;
static constexpr int PIN_CLOSED = 27;

int main(int argc, char** argv) {
  bool demo_shutter = false;
  for (int i=1;i<argc;i++) {
    std::string arg(argv[i]);
    if (arg == "--json") {
      setenv("LOG_JSON", "1", 1);
    } else if (arg == "--demo-shutter") {
      demo_shutter = true;
    }
  }

  if (demo_shutter) {
    try {
      auto chip = hal::make_chip();
      Stepper step(*chip, PIN_STEP, PIN_DIR, PIN_EN, PIN_OPEN, PIN_CLOSED, 40, 400, 80);
#ifdef USE_MOCK_GPIO
      auto closed = chip->request_line(PIN_CLOSED, hal::Direction::In);
      auto open = chip->request_line(PIN_OPEN, hal::Direction::In);
      closed->write(true);
      open->write(true);
#endif
      ShutterFSM fsm(step, 5, 80);
      fsm.cmdHome();
      while (fsm.state() != ShutterState::CLOSED && fsm.state() != ShutterState::FAULT) fsm.tick();
      if (fsm.state() != ShutterState::CLOSED) return 1;
      for (int i=0;i<2;i++) {
        fsm.cmdOpenMm(40);
        while (fsm.state() != ShutterState::OPEN && fsm.state() != ShutterState::FAULT) fsm.tick();
        if (fsm.state() == ShutterState::FAULT) return 1;
        fsm.cmdCloseMm(40);
        while (fsm.state() != ShutterState::CLOSED && fsm.state() != ShutterState::FAULT) fsm.tick();
        if (fsm.state() == ShutterState::FAULT) return 1;
      }
      return 0;
    } catch (const std::exception& e) {
      LOG_ERROR("shutter_demo", {{"err", e.what()}});
      return 1;
    }
  }

  auto chip = hal::make_mock_chip();
  (void)chip; // unused in this simple sim
  LOG_INFO("sim_start");
  auto sim = make_default_sim();
  sim.run();
  const auto& st = sim.state();
  LOG_INFO("sim_end", {{"pulses", std::to_string(st.hopper_pulses)},
                        {"closed", st.endstop_closed?"1":"0"},
                        {"open", st.endstop_open?"1":"0"},
                        {"scale_delta", std::to_string(st.scale_raw - st.scale_base)}});
  std::cout << "SIM_OK pulses=" << st.hopper_pulses
            << " closed=" << (st.endstop_closed?1:0)
            << " open=" << (st.endstop_open?1:0)
            << " scale_delta=" << (st.scale_raw - st.scale_base)
            << std::endl;
  return 0;
}

#include "drivers/stepper.hpp"
#include "drivers/hopper_parallel.hpp"
#include "drivers/hx711.hpp"
#include "hal/gpio.hpp"
#include <iostream>

// ==== PIN MAP (BCM or "GPIO line numbers" depending on chip, adjust to your wiring) ====
// libgpiod numbering refers to the "line offset" on gpiochip0. Update these to match your Pi header mapping.
static constexpr int PIN_STEP      = 23;  // stepper STEP
static constexpr int PIN_DIR       = 24;  // stepper DIR
static constexpr int PIN_ENABLE    = 25;  // stepper EN (active-low)
static constexpr int PIN_LIMIT_O   = 26;  // OPEN limit switch (true when pressed)
static constexpr int PIN_LIMIT_C   = 27;  // CLOSED limit switch

static constexpr int PIN_HOPPER_EN = 5;   // drive MOSFET/relay for hopper motor (true=on)
static constexpr int PIN_HOPPER_P  = 6;   // optic pulse input (TTL via opto/level shifter)

static constexpr int PIN_HX_DT     = 19;  // HX711 DT
static constexpr int PIN_HX_SCK    = 13;  // HX711 SCK

int main() {
  try {
    auto chip = hal::make_chip("gpiochip0");

    // Init drivers
    Stepper shutter(*chip, PIN_STEP, PIN_DIR, PIN_ENABLE, PIN_LIMIT_O, PIN_LIMIT_C,
                    /*steps_per_mm=*/40, /*pulse_us=*/400);
    HopperParallel hopper(*chip, PIN_HOPPER_EN, PIN_HOPPER_P, /*pulses_per_coin=*/1);
    HX711 scale(*chip, PIN_HX_DT, PIN_HX_SCK);

    // --- Demo sequence ---
    std::cerr << "[BOOT] Homing shutter to CLOSED...\n";
    if (!shutter.home_closed(/*max_mm=*/80)) {
      std::cerr << "[FAIL] Could not home shutter. Check wiring/limits.\n";
      return 1;
    }

    std::cerr << "[STEP] Opening shutter 40mm for self-test...\n";
    shutter.open_mm(40);
    hal::sleep_us(300000);
    std::cerr << "[STEP] Closing shutter...\n";
    shutter.close_mm(40);

    // Read scale baseline
    long base = 0;
    try { base = scale.read_average(4); std::cerr << "[SCALE] Baseline="<< base << "\n"; }
    catch(...) { std::cerr << "[SCALE] Skipping scale (not wired?)\n"; }

    // Dispense change: 3 quarters
    std::cerr << "[HOPPER] Dispensing 3 coins...\n";
    if (!hopper.dispense(3, /*max_ms=*/5000)) {
      std::cerr << "[FAIL] Hopper timeout or sensor issue.\n";
    }

    // Open shutter to present change
    shutter.open_mm(40);
    hal::sleep_us(2000000); // 2s for customer to take coins
    shutter.close_mm(40);

    // Audit weight delta
    try {
      long post = scale.read_average(4);
      std::cerr << "[SCALE] Delta=" << (post - base) << " (raw units)\n";
    } catch(...) {}

    std::cerr << "[DONE] Demo OK\n";
    return 0;
  } catch (const std::exception& e) {
    std::cerr << "[EXC] " << e.what() << "\n";
    return 2;
  }
}
