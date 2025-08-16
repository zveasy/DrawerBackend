#include <gtest/gtest.h>
#include "cloud/identity.hpp"
#include "config/config.hpp"
#include <cstdlib>

TEST(Identity, FallbackDeterministic) {
  cfg::Config c = cfg::defaults();
  c.identity.use_tpm = false;
  c.identity.device_id_prefix = "REG";
  setenv("RMVP_CPU_SERIAL", "abc", 1);
  setenv("RMVP_MACHINE_ID", "123", 1);
  std::string id1 = cloud::device_id(c);
  std::string id2 = cloud::device_id(c);
  EXPECT_EQ(id1, id2);
  EXPECT_EQ(0u, id1.find("REG"));
}
