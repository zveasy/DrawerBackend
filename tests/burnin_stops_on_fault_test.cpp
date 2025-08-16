#include "burnin/burnin.hpp"
#include <gtest/gtest.h>
#include <fstream>
#include <filesystem>

TEST(Burnin, StopsOnFault) {
  cfg::Config cfg = cfg::load();
  cfg.burnin.cycles = 10;
  cfg.burnin.log_path = "burnin.log";
  std::filesystem::remove(cfg.burnin.log_path);
  int cnt=0;
  auto fault = [&](int){ return ++cnt==3; };
  int rc = burnin::run(cfg, fault);
  EXPECT_NE(rc,0);
  std::ifstream in(cfg.burnin.log_path); std::string last, line; while(std::getline(in,line)) last=line;
  EXPECT_NE(last.find("\"fault\":true"), std::string::npos);
  std::remove(cfg.burnin.log_path.c_str());
}
