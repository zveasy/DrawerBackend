#include <gtest/gtest.h>
#include <filesystem>
#include "../src/app/txn_engine.hpp"

struct FakeShutter : IShutter {
  int open_calls{0};
  bool home(int, std::string*) override { return true; }
  bool open_mm(int, std::string*) override { ++open_calls; return true; }
  bool close_mm(int, std::string*) override { return true; }
};

struct FailDispenser : IDispenser {
  int last_req{0};
  DispenseStats dispenseCoins(int coins) override {
    last_req = coins;
    return DispenseStats{false, coins, 0, 0, 0, "JAM", 0};
  }
};

TEST(TxnRecovery, VoidOnFailure) {
  std::filesystem::remove_all("data");
  journal::Txn t; t.id="txn-test"; t.quarters=8; t.dispensed=3; t.phase="DISPENSING"; journal::append(t);
  FakeShutter sh; FailDispenser disp; TxnConfig cfg; TxnEngine eng(sh, disp, cfg);
  auto r = eng.resume_if_needed();
  EXPECT_EQ("VOID", r.phase);
  EXPECT_EQ(0, sh.open_calls);
  EXPECT_EQ(5, disp.last_req);
  EXPECT_NE("", r.reason);
}
