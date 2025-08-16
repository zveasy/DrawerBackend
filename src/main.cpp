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
