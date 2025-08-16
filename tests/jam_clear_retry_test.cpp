#include <gtest/gtest.h>
#include <filesystem>
#include "../src/app/service_mode.hpp"
#include "../src/safety/faults.hpp"
#include "../src/util/event_log.hpp"

struct JamDisp : IDispenser {
  bool jammed{true};
  int nudges{0};
  DispenseStats dispenseCoins(int coins) override {
    if(coins==0){ jammed=false; ++nudges; return {true,0,0,0,0,"",0}; }
    if(jammed){ return {false,0,0,0,0,"JAM",0}; }
    return {true,coins,coins,0,0,"",0};
  }
};

TEST(JamClear, RetrySecond) {
  std::filesystem::remove_all("data");
  eventlog::Logger elog("data/service.log");
  cfg::Service scfg; scfg.enable=true; scfg.pin_code="1234"; scfg.hopper_max_retries=2;
  safety::FaultManager fm(cfg::Safety{}, &elog); fm.start();
  ServiceMode svc(scfg, fm, elog);
  ASSERT_TRUE(svc.enter("1234"));
  JamDisp d;
  auto st1 = d.dispenseCoins(1);
  EXPECT_FALSE(st1.ok);
  fm.raise(safety::FaultCode::JAM_HOPPER, "no pulses");
  EXPECT_TRUE(svc.run_jam_clear_hopper(d, scfg));
  auto st2 = d.dispenseCoins(1);
  EXPECT_TRUE(st2.ok);
  EXPECT_FALSE(fm.any_active());
  fm.stop();
}

