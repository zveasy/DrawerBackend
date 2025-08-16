#include <gtest/gtest.h>
#include <thread>
#include <filesystem>
#include "../src/pos/serial_pos.hpp"
#include "../src/pos/router.hpp"
#include "../src/pos/idempotency_store.hpp"

struct FakeShutter : IShutter {
  bool home(int, std::string*) override { return true; }
  bool open_mm(int, std::string*) override { return true; }
  bool close_mm(int, std::string*) override { return true; }
};

struct SlowDisp : IDispenser {
  DispenseStats dispenseCoins(int c) override {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    return DispenseStats{true, c, c, 0, 0, "", 0};
  }
};

TEST(PosSerialContract, Lines) {
  std::filesystem::remove_all("idemdata");
  FakeShutter sh; SlowDisp disp; TxnConfig cfg; TxnEngine eng(sh, disp, cfg);
  pos::IdempotencyStore store("idemdata"); store.open();
  pos::Router router(eng, store);
  pos::SerialConnector conn(router);

  // OK path with flexible whitespace and order
  std::string ok = conn.handle_line("REQ   deposit=200   id=abc  price=100\n");
  EXPECT_NE(std::string::npos, ok.find("OK id=abc"));

  // Busy with different id concurrent
  std::string r1, r2;
  auto f1 = [&]{ r1 = conn.handle_line("REQ id=x1 price=100 deposit=200"); };
  auto f2 = [&]{ r2 = conn.handle_line("REQ id=x2 price=100 deposit=200"); };
  std::thread t1(f1), t2(f2); t1.join(); t2.join();
  EXPECT_TRUE((r1.find("BUSY")!=std::string::npos && r2.find("OK")!=std::string::npos) ||
              (r2.find("BUSY")!=std::string::npos && r1.find("OK")!=std::string::npos));

  // Pending + conflict
  std::thread t3([&]{ conn.handle_line("REQ id=p1 price=100 deposit=200"); });
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  std::string pending = conn.handle_line("REQ id=p1 price=100 deposit=200");
  EXPECT_NE(std::string::npos, pending.find("PENDING"));
  t3.join();
  std::string conflict = conn.handle_line("REQ id=p1 price=150 deposit=200");
  EXPECT_NE(std::string::npos, conflict.find("CONFLICT"));
}

