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
  int last_req{0};
  DispenseStats next{true,0,4,0,0,"",0};
  DispenseStats dispenseCoins(int coins) override {
    last_req = coins;
    next.requested = coins;
    return next;
  }
};

TEST(TxnRecovery, Resume) {
  std::filesystem::remove_all("data");
  journal::Txn t; t.id="txn-test"; t.quarters=10; t.dispensed=6; t.phase="DISPENSING"; journal::append(t);
  FakeShutter sh; FakeDispenser disp; TxnConfig cfg; TxnEngine eng(sh, disp, cfg);
  auto r = eng.resume_if_needed();
  EXPECT_EQ("DONE", r.phase);
  EXPECT_EQ(1, sh.open_calls);
  EXPECT_EQ(1, sh.close_calls);
  EXPECT_EQ(4, disp.last_req);
  EXPECT_EQ(10, r.dispensed);
}
