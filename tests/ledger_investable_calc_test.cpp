#include <gtest/gtest.h>
#include "../src/quant/ledger.hpp"

using quant::Snapshot;
using quant::investable_cents;

TEST(LedgerInvestable, Table) {
  struct Case { Snapshot s; int64_t want; };
  std::vector<Case> cases = {
    {{10000,0,2000}, 8000},
    {{15000,500,2000}, 12500},
    {{2000,0,2000}, 0},
    {{5000,4000,2000}, 0}
  };
  for (const auto& c : cases) {
    EXPECT_EQ(investable_cents(c.s), c.want);
  }
}
