#include <gtest/gtest.h>

#include "../src/app/audit.hpp"
#include "../src/util/cli_options.hpp"

TEST(AuditSkip, NullScale) {
  audit::Config cfg;
  auto res = audit::run(nullptr, cfg, 3);
  EXPECT_TRUE(res.skipped);
}

TEST(AuditSkip, CliSkips) {
  const char* argv[] = {"register_mvp", "dispense", "3", "--json"};
  CliOptions opts;
  ASSERT_EQ(0, opts.parse(4, const_cast<char**>(argv)));
  EXPECT_TRUE(opts.log_json);
  EXPECT_EQ(3, opts.dispense_n);
  EXPECT_FALSE(opts.use_scale);

  audit::Config cfg;
  auto res = audit::run(nullptr, cfg, opts.dispense_n);
  EXPECT_TRUE(res.skipped);
}
