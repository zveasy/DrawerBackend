#include <gtest/gtest.h>
#include "ota/agent.hpp"

TEST(OtaRingSelection, Deterministic) {
  std::string good, bad;
  for (int i = 0; i < 10000; ++i) {
    std::string id = "dev" + std::to_string(i);
    int h = ota::Agent::hash_device(id);
    if (h < 10 && good.empty()) good = id;
    if (h >= 10 && bad.empty()) bad = id;
    if (!good.empty() && !bad.empty()) break;
  }
  ASSERT_FALSE(good.empty());
  ASSERT_FALSE(bad.empty());
  EXPECT_TRUE(ota::Agent::allow(ota::Agent::hash_device(good), 10));
  EXPECT_FALSE(ota::Agent::allow(ota::Agent::hash_device(bad), 10));
}

