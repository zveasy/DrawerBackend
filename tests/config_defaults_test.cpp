#include "config/config.hpp"
#include <gtest/gtest.h>
#include <fstream>
#include <cstdio>
#include <cstdlib>

TEST(Config, DefaultsApplied) {
  unsetenv("REGISTER_MVP_CONFIG");
  auto c = cfg::load();
  const auto& d = cfg::defaults();
  EXPECT_EQ(c.mech.steps_per_mm, d.mech.steps_per_mm);
  EXPECT_EQ(c.pins.step, d.pins.step);
}

TEST(Config, PartialMerge) {
  std::string path = "test_partial.ini";
  {
    std::ofstream f(path);
    f << "[mechanics]\nsteps_per_mm=200\n";
  }
  setenv("REGISTER_MVP_CONFIG", path.c_str(), 1);
  auto c = cfg::load();
  const auto& d = cfg::defaults();
  EXPECT_EQ(c.mech.steps_per_mm, 200);
  EXPECT_EQ(c.mech.open_mm, d.mech.open_mm);
  unsetenv("REGISTER_MVP_CONFIG");
  std::remove(path.c_str());
}

TEST(Config, InvalidValueWarning) {
  std::string path = "test_invalid.ini";
  {
    std::ofstream f(path);
    f << "[mechanics]\nsteps_per_mm=abc\n";
  }
  setenv("REGISTER_MVP_CONFIG", path.c_str(), 1);
  auto c = cfg::load();
  const auto& d = cfg::defaults();
  EXPECT_EQ(c.mech.steps_per_mm, d.mech.steps_per_mm);
  bool found=false;
  for (const auto& w : c.warnings) {
    if (w.find("mechanics.steps_per_mm") != std::string::npos) found = true;
  }
  EXPECT_TRUE(found);
  unsetenv("REGISTER_MVP_CONFIG");
  std::remove(path.c_str());
}
