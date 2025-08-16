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
  int count{0};
  DispenseStats dispenseCoins(int c) override {
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    count += c;
    return DispenseStats{true, c, c, 0, 0, "", 0};
  }
};

TEST(PosIdempotency, RaceDuplicate) {
  std::filesystem::remove_all("idemdata");
  FakeShutter sh; SlowDisp disp; TxnConfig cfg; TxnEngine eng(sh, disp, cfg);
  pos::IdempotencyStore store("idemdata"); store.open();
  pos::Router router(eng, store);
  pos::Options opt; opt.port = 0; opt.vendor_mode = "A";
  pos::HttpConnector conn(router, opt); ASSERT_TRUE(conn.start());
  int port = conn.port();

  auto send = [&](int &status){
    httplib::Client cli("127.0.0.1", port);
    httplib::Headers h{{"X-Idempotency-Key","race"}};
    std::string body = "{\"price_cents\":100,\"deposit_cents\":200}";
    auto r = cli.Post("/pos/purchase", h, body, "application/json");
    status = r ? r->status : 0;
  };
  int s1=0,s2=0;
  std::thread t1(send,std::ref(s1));
  std::thread t2(send,std::ref(s2));
  t1.join(); t2.join();
  EXPECT_NE(s1,0); EXPECT_NE(s2,0);
  EXPECT_TRUE((s1==200 && s2==202) || (s1==202 && s2==200));
  EXPECT_EQ(4, disp.count);

  // After completion, replay should return cached 200
  httplib::Client cli("127.0.0.1", port);
  httplib::Headers h{{"X-Idempotency-Key","race"}};
  std::string body = "{\"price_cents\":100,\"deposit_cents\":200}";
  auto r3 = cli.Post("/pos/purchase", h, body, "application/json");
  ASSERT_TRUE(r3); EXPECT_EQ(200, r3->status);
  EXPECT_EQ(4, disp.count);
  conn.stop();
}

