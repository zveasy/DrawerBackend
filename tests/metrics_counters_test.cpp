#include <gtest/gtest.h>
#include "obs/metrics.hpp"

using namespace obs;

TEST(MetricsCounters, Basic) {
  auto& m = M();
  m.counter("register_txn_total","",{{"status","OK"}}).inc();
  m.counter("register_txn_total","",{{"status","VOID"}}).inc();
  m.counter("register_jam_total","",{{"type","hopper"}}).inc();
  m.counter("register_dispense_coins_total","",{{"denom","quarter"}}).inc(3);

  EXPECT_EQ(1, m.counter("register_txn_total","",{{"status","OK"}}).value());
  EXPECT_EQ(1, m.counter("register_txn_total","",{{"status","VOID"}}).value());
  EXPECT_EQ(1, m.counter("register_jam_total","",{{"type","hopper"}}).value());
  EXPECT_EQ(3, m.counter("register_dispense_coins_total","",{{"denom","quarter"}}).value());
}
