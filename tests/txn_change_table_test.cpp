#include <gtest/gtest.h>
#include <filesystem>
#include "../src/app/txn_engine.hpp"

struct FakeShutter : IShutter {
  int open_calls{0}, close_calls{0};
  bool home(int, std::string*) override { return true; }
  bool open_mm(int, std::string*) override { ++open_calls; return true; }
  bool close_mm(int, std::string*) override { ++close_calls; return true; }
};

struct FakeDispenser : IDispenser {
  int last_requested{0};
  DispenseStats dispenseCoins(int coins) override {
    last_requested = coins;
    return DispenseStats{true, coins, coins, 0, 0, "", 0};
  }
};

TEST(TxnChangeTable, Quarters) {
  std::filesystem::remove_all("data");
  FakeShutter sh;
  FakeDispenser disp;
  TxnConfig cfg;
  TxnEngine eng(sh, disp, cfg);
  for (int q = 0; q <= 10; ++q) {
    int price = 100;
    int deposit = 100 + q * 25;
    auto res = eng.run_purchase(price, deposit);
    EXPECT_EQ(q, disp.last_requested);
    EXPECT_EQ(q, res.quarters);
    EXPECT_EQ("DONE", res.phase);
    journal::Txn jt; ASSERT_TRUE(journal::load_last(jt));
    EXPECT_EQ("DONE", jt.phase);
    EXPECT_EQ(q, jt.quarters);
  }
}
