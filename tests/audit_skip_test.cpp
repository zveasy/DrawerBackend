#include <gtest/gtest.h>
#include <cstdio>
#include <string>

#include "../src/app/audit.hpp"

TEST(AuditSkip, NullScale) {
  audit::Config cfg;
  auto res = audit::run(nullptr, cfg, 3);
  EXPECT_TRUE(res.skipped);
}

TEST(AuditSkip, CliSkips) {
  // Use the flag form "--dispense 3" so CliOptions parses it regardless of argument order.
  FILE* fp = popen("../register_mvp --json --dispense 3", "r");
  ASSERT_NE(fp, nullptr);
  char buf[256];
  std::string out;
  while (fgets(buf, sizeof(buf), fp)) out += buf;
  int rc = pclose(fp);
  EXPECT_NE(rc, -1);
  EXPECT_NE(out.find("\"skipped\":true"), std::string::npos);
}
