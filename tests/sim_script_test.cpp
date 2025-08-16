#include <gtest/gtest.h>
#include "../sim/sim.hpp"

TEST(SimScript, Counts) {
  Simulator sim;
  sim.add_event(0, [](SimState& s){ s.endstop_closed=true; });
  sim.add_event(10, [](SimState& s){ s.hopper_sig=!s.hopper_sig; s.hopper_pulses++; });
  sim.add_event(20, [](SimState& s){ s.scale_raw=42; });
  sim.run();
  const auto& st = sim.state();
  EXPECT_EQ(st.hopper_pulses,1);
  EXPECT_TRUE(st.endstop_closed);
  EXPECT_EQ(st.scale_raw,42);
}
