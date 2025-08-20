#include "eol/eol_runner.hpp"
#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>

struct MockShutter : IShutter {
  bool home(int,std::string*) override { return true; }
  bool open_mm(int,std::string*) override { return true; }
  bool close_mm(int,std::string*) override { return true; }
};

struct MockDisp : IDispenser {
  DispenseStats dispenseCoins(int coins) override {
    return {true, coins, coins, 0,0,"",0};
  }
};

struct MockScale : IScale {
  long base{100};
  bool first{true};
  long read_average(int) override {
    if(first){first=false; return base;} else return base+17010; }
};

TEST(EolRunner, Success) {
  auto lr = cfg::load();
  ASSERT_TRUE(lr.errors.empty());
  cfg::Config cfg = lr.config;
  cfg.eol.result_dir = "eol_tmp";
  std::filesystem::remove_all(cfg.eol.result_dir);
  MockShutter sh; MockDisp disp; MockScale sc;
  std::ofstream("telemetry.log") << "status=OK";
  eol::Runner r(sh, disp, &sc, "telemetry.log");
  auto res = r.run_once(cfg);
  EXPECT_TRUE(res.pass);
  EXPECT_EQ(res.dispensed, cfg.eol.coins_for_test);
  EXPECT_TRUE(std::filesystem::exists(res.report_path));
  EXPECT_TRUE(std::filesystem::exists(std::filesystem::path(res.report_path).replace_filename("result.csv")));
  std::filesystem::remove_all(cfg.eol.result_dir);
  std::remove("telemetry.log");
}
