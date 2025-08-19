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

TEST(Config, OverridesAllSections) {
  std::string path = "test_overrides.ini";
  {
    std::ofstream f(path);
    f << "[io.pins]\nstep=1\n";
    f << "[mechanics]\nsteps_per_mm=200\n";
    f << "[hopper]\npulses_per_coin=2\n";
    f << "[dispense]\njam_ms=111\n";
    f << "[audit]\ncoin_mass_g=1.23\n";
    f << "[presentation]\npresent_ms=333\n";
    f << "[selftest]\nenable_coin_test=0\n";
    f << "[aws]\nenable=1\nendpoint=http://example\n";
    f << "[security]\nenable_ro_root=1\n";
    f << "[identity]\nuse_tpm=1\n";
    f << "[safety]\nestop_pin=5\n";
    f << "[service]\nenable=0\n";
    f << "[pos]\nenable_http=0\n";
    f << "[ota]\nenable=1\n";
    f << "[manufacturing]\nenable_first_boot_eol=0\n";
    f << "[eol]\nweigh_tolerance_g=0.5\n";
    f << "[burnin]\ncycles=1000\n";
    f << "[quant]\nenable=1\n";
  }
  setenv("REGISTER_MVP_CONFIG", path.c_str(), 1);
  auto c = cfg::load();
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
  std::string path = "test_invalid_all.ini";
  {
    std::ofstream f(path);
    f << "[io.pins]\nstep=abc\n";
    f << "[mechanics]\nsteps_per_mm=abc\n";
    f << "[hopper]\npulses_per_coin=abc\n";
    f << "[dispense]\njam_ms=abc\n";
    f << "[audit]\ncoin_mass_g=abc\n";
    f << "[presentation]\npresent_ms=abc\n";
    f << "[selftest]\nenable_coin_test=abc\n";
    f << "[aws]\nenable=abc\n";
    f << "[security]\nenable_ro_root=abc\n";
    f << "[identity]\nuse_tpm=abc\n";
    f << "[safety]\nestop_pin=abc\n";
    f << "[service]\nenable=abc\n";
    f << "[pos]\nenable_http=abc\n";
    f << "[ota]\nenable=abc\n";
    f << "[manufacturing]\nenable_first_boot_eol=abc\n";
    f << "[eol]\nweigh_tolerance_g=abc\n";
    f << "[burnin]\ncycles=abc\n";
    f << "[quant]\nenable=abc\n";
  }
  setenv("REGISTER_MVP_CONFIG", path.c_str(), 1);
  auto c = cfg::load();
  const auto& d = cfg::defaults();
  EXPECT_EQ(c.pins.step, d.pins.step);
  EXPECT_EQ(c.mech.steps_per_mm, d.mech.steps_per_mm);
  EXPECT_EQ(c.hopper.pulses_per_coin, d.hopper.pulses_per_coin);
  EXPECT_EQ(c.disp.jam_ms, d.disp.jam_ms);
  EXPECT_DOUBLE_EQ(c.audit.coin_mass_g, d.audit.coin_mass_g);
  EXPECT_EQ(c.pres.present_ms, d.pres.present_ms);
  EXPECT_EQ(c.st.enable_coin_test, d.st.enable_coin_test);
  EXPECT_EQ(c.aws.enable, d.aws.enable);
  EXPECT_EQ(c.security.enable_ro_root, d.security.enable_ro_root);
  EXPECT_EQ(c.identity.use_tpm, d.identity.use_tpm);
  EXPECT_EQ(c.safety.estop_pin, d.safety.estop_pin);
  EXPECT_EQ(c.service.enable, d.service.enable);
  EXPECT_EQ(c.pos.enable_http, d.pos.enable_http);
  EXPECT_EQ(c.ota.enable, d.ota.enable);
  EXPECT_EQ(c.mfg.enable_first_boot_eol, d.mfg.enable_first_boot_eol);
  EXPECT_DOUBLE_EQ(c.eol.weigh_tolerance_g, d.eol.weigh_tolerance_g);
  EXPECT_EQ(c.burnin.cycles, d.burnin.cycles);
  EXPECT_EQ(c.quant.enable, d.quant.enable);
  std::vector<std::string> keys = {
    "io.pins.step","mechanics.steps_per_mm","hopper.pulses_per_coin","dispense.jam_ms",
    "audit.coin_mass_g","presentation.present_ms","selftest.enable_coin_test","aws.enable",
    "security.enable_ro_root","identity.use_tpm","safety.estop_pin","service.enable",
    "pos.enable_http","ota.enable","manufacturing.enable_first_boot_eol","eol.weigh_tolerance_g",
    "burnin.cycles","quant.enable"};
  for (const auto& k : keys) {
    bool found = false;
    for (const auto& w : c.warnings) {
      if (w.find(k) != std::string::npos) { found = true; break; }
    }
    EXPECT_TRUE(found) << k;
  }
  unsetenv("REGISTER_MVP_CONFIG");
  std::remove(path.c_str());
}
