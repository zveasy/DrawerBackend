#include "provision/serial.hpp"
#include <gtest/gtest.h>
#include <regex>

TEST(Serial, FormatAndSequence) {
  cfg::Config cfg = cfg::load();
  std::time_t fake = 1700000000; // fixed time
  setenv("PLANT_CODE","P1",1);
  std::string s1 = provision::make_serial(cfg, fake);
  std::string s2 = provision::make_serial(cfg, fake);
  std::regex re("REG-[0-9]{4}-P1-[0-9]{4}");
  EXPECT_TRUE(std::regex_match(s1, re));
  EXPECT_TRUE(std::regex_match(s2, re));
  EXPECT_NE(s1, s2);
}
