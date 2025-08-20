#include "config/config.hpp"
#include <gtest/gtest.h>
#include <fstream>
#include <cstdio>
#include <cstdlib>
#include <nlohmann/json.hpp>

TEST(Config, DefaultsApplied) {
  unsetenv("REGISTER_MVP_CONFIG");
  auto lr = cfg::load();
  ASSERT_TRUE(lr.errors.empty());
  const auto& c = lr.config;
  const auto& d = cfg::defaults();
  EXPECT_EQ(c.mech.steps_per_mm, d.mech.steps_per_mm);
  EXPECT_EQ(c.pins.step, d.pins.step);
}

TEST(Config, PartialMerge) {
  std::string path = "test_partial.json";
  nlohmann::json j;
  j["mechanics"]["steps_per_mm"] = 200;
  std::ofstream(path) << j.dump();
  setenv("REGISTER_MVP_CONFIG", path.c_str(), 1);
  auto lr = cfg::load();
  ASSERT_TRUE(lr.errors.empty());
  const auto& c = lr.config;
  const auto& d = cfg::defaults();
  EXPECT_EQ(c.mech.steps_per_mm, 200);
  EXPECT_EQ(c.mech.open_mm, d.mech.open_mm);
  unsetenv("REGISTER_MVP_CONFIG");
  std::remove(path.c_str());
}

TEST(Config, InvalidValueWarning) {
  std::string path = "test_invalid.json";
  nlohmann::json j;
  j["mechanics"]["steps_per_mm"] = "abc";
  std::ofstream(path) << j.dump();
  setenv("REGISTER_MVP_CONFIG", path.c_str(), 1);
  auto lr = cfg::load();
  EXPECT_FALSE(lr.errors.empty());
  bool found=false;
  for (const auto& e : lr.errors) {
    if (e.find("mechanics.steps_per_mm") != std::string::npos) found = true;
  }
  EXPECT_TRUE(found);
  unsetenv("REGISTER_MVP_CONFIG");
  std::remove(path.c_str());
}

TEST(Config, OverridesAllSections) {
  std::string path = "test_overrides.json";
  nlohmann::json j;
  j["io"]["pins"]["step"] = 1;
  j["mechanics"]["steps_per_mm"] = 200;
  j["hopper"]["pulses_per_coin"] = 2;
  j["dispense"]["jam_ms"] = 111;
  j["audit"]["coin_mass_g"] = 1.23;
  j["presentation"]["present_ms"] = 333;
  j["selftest"]["enable_coin_test"] = 0;
  j["aws"]["enable"] = 1;
  j["aws"]["endpoint"] = "http://example";
  j["security"]["enable_ro_root"] = 1;
  j["identity"]["use_tpm"] = 1;
  j["safety"]["estop_pin"] = 5;
  j["service"]["enable"] = 0;
  j["pos"]["enable_http"] = 0;
  j["ota"]["enable"] = 1;
  j["manufacturing"]["enable_first_boot_eol"] = 0;
  j["eol"]["weigh_tolerance_g"] = 0.5;
  j["burnin"]["cycles"] = 1000;
  j["quant"]["enable"] = 1;
  std::ofstream(path) << j.dump();
  setenv("REGISTER_MVP_CONFIG", path.c_str(), 1);
  auto lr = cfg::load();
  ASSERT_TRUE(lr.errors.empty());
  const auto& c = lr.config;
  EXPECT_EQ(c.pins.step, 1);
  EXPECT_EQ(c.mech.steps_per_mm, 200);
  EXPECT_EQ(c.hopper.pulses_per_coin, 2);
  EXPECT_EQ(c.disp.jam_ms, 111);
  EXPECT_DOUBLE_EQ(c.audit.coin_mass_g, 1.23);
  EXPECT_EQ(c.pres.present_ms, 333);
  EXPECT_FALSE(c.st.enable_coin_test);
  EXPECT_TRUE(c.aws.enable);
  EXPECT_EQ(c.aws.endpoint, "http://example");
  EXPECT_TRUE(c.security.enable_ro_root);
  EXPECT_TRUE(c.identity.use_tpm);
  EXPECT_EQ(c.safety.estop_pin, 5);
  EXPECT_FALSE(c.service.enable);
  EXPECT_FALSE(c.pos.enable_http);
  EXPECT_TRUE(c.ota.enable);
  EXPECT_FALSE(c.mfg.enable_first_boot_eol);
  EXPECT_DOUBLE_EQ(c.eol.weigh_tolerance_g, 0.5);
  EXPECT_EQ(c.burnin.cycles, 1000);
  EXPECT_TRUE(c.quant.enable);
  unsetenv("REGISTER_MVP_CONFIG");
  std::remove(path.c_str());
}

TEST(Config, InvalidValuesAllSections) {
  std::string path = "test_invalid_all.json";
  nlohmann::json j;
  j["io"]["pins"]["step"] = "abc";
  j["mechanics"]["steps_per_mm"] = "abc";
  j["hopper"]["pulses_per_coin"] = "abc";
  j["dispense"]["jam_ms"] = "abc";
  j["audit"]["coin_mass_g"] = "abc";
  j["presentation"]["present_ms"] = "abc";
  j["selftest"]["enable_coin_test"] = "abc";
  j["aws"]["enable"] = "abc";
  j["security"]["enable_ro_root"] = "abc";
  j["identity"]["use_tpm"] = "abc";
  j["safety"]["estop_pin"] = "abc";
  j["service"]["enable"] = "abc";
  j["pos"]["enable_http"] = "abc";
  j["ota"]["enable"] = "abc";
  j["manufacturing"]["enable_first_boot_eol"] = "abc";
  j["eol"]["weigh_tolerance_g"] = "abc";
  j["burnin"]["cycles"] = "abc";
  j["quant"]["enable"] = "abc";
  std::ofstream(path) << j.dump();
  setenv("REGISTER_MVP_CONFIG", path.c_str(), 1);
  auto lr = cfg::load();
  EXPECT_FALSE(lr.errors.empty());
  std::vector<std::string> keys = {
    "io.pins.step","mechanics.steps_per_mm","hopper.pulses_per_coin","dispense.jam_ms",
    "audit.coin_mass_g","presentation.present_ms","selftest.enable_coin_test","aws.enable",
    "security.enable_ro_root","identity.use_tpm","safety.estop_pin","service.enable",
    "pos.enable_http","ota.enable","manufacturing.enable_first_boot_eol","eol.weigh_tolerance_g",
    "burnin.cycles","quant.enable"};
  for (const auto& k : keys) {
    bool found = false;
    for (const auto& e : lr.errors) {
      if (e.find(k) != std::string::npos) { found = true; break; }
    }
    EXPECT_TRUE(found) << k;
  }
  unsetenv("REGISTER_MVP_CONFIG");
  std::remove(path.c_str());
}
