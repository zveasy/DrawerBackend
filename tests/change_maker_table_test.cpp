#include <gtest/gtest.h>
#include "../src/app/change_maker.hpp"

TEST(ChangeMakerTable, BasicPlans) {
  Inventory inv;
  for(const auto& spec : default_us_specs()){
    HopperStock hs; hs.spec = spec; hs.logical_count = 100;
    inv.upsert(hs);
  }
  ChangeMaker cm(inv);
  struct Case { int cents; int q,d,n,p; } cases[] = {
    {65,2,1,1,0},
    {40,1,1,1,0},
    {7,0,0,1,2},
  };
  for(const auto& c : cases){
    int leftover=0; auto plan = cm.make_plan_cents(c.cents, leftover);
    EXPECT_EQ(0, leftover);
    int q=0,d=0,n=0,p=0;
    for(const auto& pi : plan){
      if(pi.denom==Denom::QUARTER) q=pi.count;
      else if(pi.denom==Denom::DIME) d=pi.count;
      else if(pi.denom==Denom::NICKEL) n=pi.count;
      else if(pi.denom==Denom::PENNY) p=pi.count;
    }
    EXPECT_EQ(c.q,q);
    EXPECT_EQ(c.d,d);
    EXPECT_EQ(c.n,n);
    EXPECT_EQ(c.p,p);
  }
}

