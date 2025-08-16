#include <gtest/gtest.h>
#include "../src/app/change_maker.hpp"
#include "../src/app/multi_dispenser.hpp"

struct FakeDisp : IDispenser {
  DispenseStats dispenseCoins(int coins) override {
    return DispenseStats{true, coins, coins, coins, 0, "", 0};
  }
};

TEST(LowStockFallback, SkipsLowDimes) {
  Inventory inv;
  HopperStock q{us_quarter(),30}; inv.upsert(q);
  HopperStock d{us_dime(),20}; inv.upsert(d); // at threshold -> low
  HopperStock n{us_nickel(),30}; inv.upsert(n);
  HopperStock p{us_penny(),30}; inv.upsert(p);
  ChangeMaker cm(inv);
  int leftover=0; auto plan = cm.make_plan_cents(65, leftover);
  EXPECT_EQ(0, leftover);
  int dimes=0, nickels=0;
  for(const auto& pi : plan){
    if(pi.denom==Denom::DIME) dimes=pi.count;
    if(pi.denom==Denom::NICKEL) nickels=pi.count;
  }
  EXPECT_EQ(0, dimes);
  EXPECT_EQ(3, nickels);
  MultiDispenser multi(inv);
  FakeDisp qd, dd, nd, pd;
  multi.attach({us_quarter(), &qd});
  multi.attach({us_dime(), &dd});
  multi.attach({us_nickel(), &nd});
  multi.attach({us_penny(), &pd});
  auto st = multi.execute(plan);
  EXPECT_TRUE(st.ok);
  EXPECT_EQ(28, inv.at(Denom::QUARTER).logical_count);
  EXPECT_EQ(20, inv.at(Denom::DIME).logical_count);
  EXPECT_EQ(27, inv.at(Denom::NICKEL).logical_count);
}

