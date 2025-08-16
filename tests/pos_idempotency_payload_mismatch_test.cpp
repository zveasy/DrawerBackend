#include <gtest/gtest.h>
#include <filesystem>
#include <httplib.h>
#include "../src/pos/http_pos.hpp"
#include "../src/pos/router.hpp"
#include "../src/pos/idempotency_store.hpp"

struct FakeShutter : IShutter {
  bool home(int, std::string*) override { return true; }
  bool open_mm(int, std::string*) override { return true; }
  bool close_mm(int, std::string*) override { return true; }
};

struct FakeDisp : IDispenser {
  int count{0};
  DispenseStats dispenseCoins(int c) override {
    count += c;
    return DispenseStats{true, c, c, 0, 0, "", 0};
  }
};

TEST(PosIdempotency, PayloadMismatch) {
  std::filesystem::remove_all("idemdata");
  FakeShutter sh; FakeDisp disp; TxnConfig cfg; TxnEngine eng(sh, disp, cfg);
  pos::IdempotencyStore store("idemdata"); store.open();
  pos::Router router(eng, store);
  pos::Options opt; opt.port = 0; opt.vendor_mode = "A";
  pos::HttpConnector conn(router, opt); ASSERT_TRUE(conn.start());
  int port = conn.port();
  httplib::Client cli("127.0.0.1", port);
  httplib::Headers h{{"X-Idempotency-Key","same"}};
  std::string body1 = "{\"price_cents\":100,\"deposit_cents\":200}";
  std::string body2 = "{\"price_cents\":150,\"deposit_cents\":200}";
  auto r1 = cli.Post("/pos/purchase", h, body1, "application/json");
  ASSERT_TRUE(r1); EXPECT_EQ(200, r1->status);
  auto r2 = cli.Post("/pos/purchase", h, body2, "application/json");
  ASSERT_TRUE(r2); EXPECT_EQ(409, r2->status);
  EXPECT_NE(std::string::npos, r2->body.find("idempotency_mismatch"));
  EXPECT_EQ(4, disp.count); // first only
  conn.stop();
}

