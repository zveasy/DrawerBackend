#include <gtest/gtest.h>
#include <fstream>
#include <sstream>

TEST(SystemdUnit, Hardened) {
  std::ifstream in("../../packaging/systemd/register-mvp.service");
  ASSERT_TRUE(in.good());
  std::stringstream ss; ss << in.rdbuf();
  std::string c = ss.str();
  EXPECT_NE(c.find("NoNewPrivileges=true"), std::string::npos);
  EXPECT_NE(c.find("ProtectSystem=strict"), std::string::npos);
  EXPECT_NE(c.find("CapabilityBoundingSet="), std::string::npos);
  EXPECT_NE(c.find("RestrictAddressFamilies=AF_UNIX AF_INET AF_INET6"), std::string::npos);
  EXPECT_NE(c.find("User=registermvp"), std::string::npos);
}
