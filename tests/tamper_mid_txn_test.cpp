#include <gtest/gtest.h>
#include <thread>
#include <fstream>
#include <sstream>
#include <filesystem>
#include "../hal/gpio.hpp"
#include "../src/app/txn_engine.hpp"
#include "../src/safety/faults.hpp"
#include "../src/util/event_log.hpp"

struct FakeShutter : IShutter {
  int open_calls{0};
  bool home(int,std::string*) override { return true; }
  bool open_mm(int,std::string*) override { ++open_calls; return true; }
  bool close_mm(int,std::string*) override { return true; }
};

struct FakeDispenser : IDispenser {
  DispenseStats dispenseCoins(int coins) override {
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    return DispenseStats{true,coins,coins,0,0,"",0};
  }
};

TEST(Tamper, MidTxn) {
  std::filesystem::remove_all("data");
  eventlog::Logger elog("data/service.log");
  cfg::Safety scfg; scfg.lid_tamper_pin = 20; scfg.debounce_ms=1; scfg.active_high=true;
  safety::FaultManager fm(scfg, &elog); fm.start();
  hal::mock::set_input(20,false);
  FakeShutter sh; FakeDispenser disp; TxnConfig tcfg; tcfg.open_mm=10;
  TxnEngine eng(sh, disp, tcfg, &fm);
  journal::Txn res;
  std::thread th([&]{ res = eng.run_purchase(100,125); });
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  hal::mock::set_input(20,true);
  th.join();
  EXPECT_EQ("VOID", res.phase);
  EXPECT_EQ(0, sh.open_calls);
  auto lines = std::string();
  {
    std::ifstream f("data/service.log");
    std::stringstream ss; ss<<f.rdbuf(); lines=ss.str();
  }
  EXPECT_NE(lines.find("LID_TAMPER"), std::string::npos);
  fm.stop();
}

