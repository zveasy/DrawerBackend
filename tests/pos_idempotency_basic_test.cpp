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

TEST(PosIdempotency, BasicReplay) {
  std::filesystem::remove_all("idemdata");
  FakeShutter sh; FakeDisp disp; TxnConfig cfg; TxnEngine eng(sh, disp, cfg);
  pos::IdempotencyStore store("idemdata"); ASSERT_TRUE(store.open());
  pos::Router router(eng, store);
  pos::Options opt; opt.port = 0; opt.bind = "127.0.0.1"; opt.vendor_mode = "A";
  pos::HttpConnector conn(router, opt); ASSERT_TRUE(conn.start());
  int port = conn.port();
  httplib::Client cli("127.0.0.1", port);
  std::string body = "{\"price_cents\":100,\"deposit_cents\":200}";
  httplib::Headers h{{"X-Idempotency-Key","k1"}};
  auto r1 = cli.Post("/pos/purchase", h, body, "application/json");
  ASSERT_TRUE(r1); EXPECT_EQ(200, r1->status);
  auto r2 = cli.Post("/pos/purchase", h, body, "application/json");
  ASSERT_TRUE(r2); EXPECT_EQ(200, r2->status);
  EXPECT_EQ(r1->body, r2->body);
  EXPECT_EQ(4, disp.count); // only once (4 quarters)
  conn.stop();
}

