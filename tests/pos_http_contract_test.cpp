#include <gtest/gtest.h>
#include <filesystem>
#include <httplib.h>
#include <thread>
#include "../src/pos/http_pos.hpp"
#include "../src/pos/router.hpp"
#include "../src/pos/idempotency_store.hpp"

struct FakeShutter : IShutter {
  bool home(int, std::string*) override { return true; }
  bool open_mm(int, std::string*) override { return true; }
  bool close_mm(int, std::string*) override { return true; }
};

struct SlowDisp : IDispenser {
  DispenseStats dispenseCoins(int c) override {
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    return DispenseStats{true, c, c, 0, 0, "", 0};
  }
};

TEST(PosHttpContract, Responses) {
  std::filesystem::remove_all("idemdata");
  FakeShutter sh; SlowDisp disp; TxnConfig cfg; TxnEngine eng(sh, disp, cfg);
  pos::IdempotencyStore store("idemdata"); store.open();
  pos::Router router(eng, store);
  pos::Options opt; opt.port = 0; opt.shared_key = "sek"; opt.vendor_mode="A";
  pos::HttpConnector conn(router, opt); ASSERT_TRUE(conn.start());
  int port = conn.port();
  httplib::Client cli("127.0.0.1", port);

  // Unauthorized
  auto bad = cli.Post("/pos/purchase", "{}", "application/json");
  ASSERT_TRUE(bad); EXPECT_EQ(401, bad->status);

  // Bad request
  httplib::Headers hh{{"X-Pos-Key","sek"},{"X-Idempotency-Key","k1"}};
  auto bad2 = cli.Post("/pos/purchase", hh, "{}", "application/json");
  ASSERT_TRUE(bad2); EXPECT_EQ(400, bad2->status);

  // Success
  std::string body = "{\"price_cents\":100,\"deposit_cents\":200}";
  auto ok = cli.Post("/pos/purchase", hh, body, "application/json");
  ASSERT_TRUE(ok); EXPECT_EQ(200, ok->status);
  EXPECT_NE(std::string::npos, ok->body.find("change_cents"));

  // Busy during concurrent different key
  auto send = [&](int &status){
    httplib::Client c("127.0.0.1", port);
    httplib::Headers h{{"X-Pos-Key","sek"},{"X-Idempotency-Key",std::to_string(status)}};
    std::string b = "{\"price_cents\":100,\"deposit_cents\":200}";
    auto r = c.Post("/pos/purchase", h, b, "application/json");
    status = r ? r->status : 0;
  };
  int s1=1,s2=2; std::thread t1(send,std::ref(s1)); std::thread t2(send,std::ref(s2));
  t1.join(); t2.join();
  EXPECT_TRUE((s1==200 && s2==409) || (s1==409 && s2==200));

  conn.stop();
}

