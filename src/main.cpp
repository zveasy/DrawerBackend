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

