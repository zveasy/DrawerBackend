#include <gtest/gtest.h>
#include <cstdio>
#include <fstream>
#include <string>

#include "../src/util/event_log.hpp"
#include "../src/safety/faults.hpp"
#include "../src/compliance/compliance_mode.hpp"
#include "../src/config/config.hpp"

TEST(ComplianceModeCLI, Flags) {
  std::remove("data/service.log");
  FILE* fp = popen("../register_mvp --compliance-mode emi_worst --prescan-cycle 2 --json", "r");
  ASSERT_NE(fp, nullptr);
  char buf[256];
  while (fgets(buf, sizeof(buf), fp)) { /* drain output */ }
  int rc = pclose(fp);
  EXPECT_EQ(rc, 0);
  std::ifstream f("data/service.log");
  ASSERT_TRUE(f.is_open());
  std::string log((std::istreambuf_iterator<char>(f)), {});
  EXPECT_NE(log.find("\"mode\":\"emi_worst\""), std::string::npos);
  size_t count=0,pos=0; while((pos=log.find("compliance_cycle",pos))!=std::string::npos){count++;pos++;}
  EXPECT_EQ(count,2u);
}

TEST(ComplianceModeAPI, EsdSafeBlocksMotion) {
  eventlog::Logger elog("data/esd_test.log");
  safety::FaultManager fm(cfg::Safety{}, &elog); fm.start();
  compliance::init(&fm, &elog);
  compliance::set_mode(compliance::ComplianceMode::ESD_SAFE);
  EXPECT_FALSE(fm.check_and_block_motion());
  fm.stop();
}
