#include "app/selftest.hpp"
#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>

struct FakeShutter : IShutter {
  bool home_ok;
  explicit FakeShutter(bool ok) : home_ok(ok) {}
  bool home(int, std::string* reason=nullptr) override {
    if (!home_ok && reason) *reason = "fail";
    return home_ok;
  }
  bool open_mm(int, std::string*) override { return true; }
  bool close_mm(int, std::string*) override { return true; }
};

struct FakeDisp : IDispenser {
  DispenseStats stats;
  DispenseStats dispenseCoins(int) override { return stats; }
};

struct FakeScale : IScale {
  long val;
  explicit FakeScale(long v) : val(v) {}
  long read_average(int) override { return val; }
};

TEST(SelfTest, Pass) {
  std::filesystem::remove_all("data");
  FakeShutter sh(true);
  FakeDisp disp; disp.stats = {true,1,1,1,0,"",10};
  FakeScale scale(123);
  cfg::Config conf = cfg::defaults();
  auto res = selftest::run(sh, disp, &scale, conf);
  EXPECT_TRUE(res.ok);
  EXPECT_TRUE(std::filesystem::exists("data/health.json"));
}

TEST(SelfTest, FailShutter) {
  std::filesystem::remove_all("data");
  FakeShutter sh(false);
  FakeDisp disp; disp.stats = {true,1,1,1,0,"",10};
  FakeScale scale(123);
  cfg::Config conf = cfg::defaults();
  auto res = selftest::run(sh, disp, &scale, conf);
  EXPECT_FALSE(res.ok);
  EXPECT_TRUE(std::filesystem::exists("data/health.json"));
  std::ifstream f("data/health.json");
  std::string json((std::istreambuf_iterator<char>(f)), {});
  EXPECT_NE(json.find("shutter"), std::string::npos);
  EXPECT_NE(json.find("fail"), std::string::npos);
  int exit_code = res.ok ? 0 : 1;
  EXPECT_NE(exit_code, 0);
}
