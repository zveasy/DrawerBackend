#include "sim/sim.hpp"
#include "hal/gpio.hpp"
#include "util/log.hpp"
#include <iostream>
#include <cstdlib>

int main(int argc, char** argv) {
  for (int i=1;i<argc;i++) {
    if (std::string(argv[i]) == "--json") {
      setenv("LOG_JSON", "1", 1);
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
