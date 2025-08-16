#include "eol/eol_runner.hpp"
#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>

struct MockShutter2 : IShutter {
  bool home(int,std::string*) override { return true; }
  bool open_mm(int,std::string*) override { return true; }
  bool close_mm(int,std::string*) override { return true; }
};

struct MockDisp2 : IDispenser {
  DispenseStats dispenseCoins(int coins) override { return {true, coins, coins,0,0,"",0}; }
};

struct BadScale : IScale {
  long base{100}; bool first{true};
  long read_average(int) override { if(first){first=false; return base;} return base+10; }
};

TEST(EolRunner, WeightFail) {
  cfg::Config cfg = cfg::load();
  cfg.eol.result_dir = "eol_tmp2";
  std::filesystem::remove_all(cfg.eol.result_dir);
  MockShutter2 sh; MockDisp2 disp; BadScale sc;
  std::ofstream("telemetry.log") << "status=OK";
  eol::Runner r(sh, disp, &sc, "telemetry.log");
  auto res = r.run_once(cfg);
  EXPECT_FALSE(res.pass);
  bool found=false;
  for(const auto& s: res.steps){ if(s.name=="Scale verify"){ found=true; EXPECT_FALSE(s.ok); EXPECT_EQ(s.reason,"UNDERPAY"); }}
  EXPECT_TRUE(found);
  std::filesystem::remove_all(cfg.eol.result_dir);
  std::remove("telemetry.log");
}
