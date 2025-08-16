#include <gtest/gtest.h>
#include "../src/app/change_maker.hpp"
#include "../src/app/multi_dispenser.hpp"

struct FakeDisp : IDispenser {
  int always_dispense{100};
  DispenseStats dispenseCoins(int coins) override {
    int d = std::min(coins, always_dispense);
    return DispenseStats{d==coins, coins, d, d, 0, d==coins?"":"PARTIAL",0};
  }
};

TEST(MultiHopperIntegration, PlanAndExecute) {
  Inventory inv;
  HopperStock q{us_quarter(),30}; inv.upsert(q);
  HopperStock d{us_dime(),30}; inv.upsert(d);
  HopperStock n{us_nickel(),30}; inv.upsert(n);
  HopperStock p{us_penny(),30}; inv.upsert(p);
  ChangeMaker cm(inv);
  int price=1235, deposit=1500;
  int change = (deposit - price) % 100;
  int leftover=0; auto plan = cm.make_plan_cents(change, leftover);
  EXPECT_EQ(0, leftover);
  ASSERT_EQ(3u, plan.size());
  EXPECT_EQ(Denom::QUARTER, plan[0].denom); EXPECT_EQ(2, plan[0].count);
  EXPECT_EQ(Denom::DIME, plan[1].denom);    EXPECT_EQ(1, plan[1].count);
  EXPECT_EQ(Denom::NICKEL, plan[2].denom);  EXPECT_EQ(1, plan[2].count);
  MultiDispenser multi(inv);
  FakeDisp dq, dd, dn, dp;
  multi.attach({us_quarter(), &dq});
  multi.attach({us_dime(), &dd});
  multi.attach({us_nickel(), &dn});
  multi.attach({us_penny(), &dp});
  auto st = multi.execute(plan);
  EXPECT_TRUE(st.ok);
  EXPECT_EQ(28, inv.at(Denom::QUARTER).logical_count);
  EXPECT_EQ(29, inv.at(Denom::DIME).logical_count);
  EXPECT_EQ(29, inv.at(Denom::NICKEL).logical_count);
  // partial quarter
  dq.always_dispense = 1;
  auto st2 = multi.execute(plan);
  EXPECT_FALSE(st2.ok);
  int fq=0;
  for(const auto& pi : st2.fulfilled) if(pi.denom==Denom::QUARTER) fq=pi.count;
  EXPECT_EQ(1, fq);
  EXPECT_EQ(27, inv.at(Denom::QUARTER).logical_count);
}

