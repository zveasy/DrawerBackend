#include <gtest/gtest.h>
#include "../src/server/http_server.hpp"
#include <httplib.h>
#include <filesystem>

struct FakeShutter : IShutter {
  bool home(int, std::string*) override { return true; }
  bool open_mm(int, std::string*) override { return true; }
  bool close_mm(int, std::string*) override { return true; }
};

struct FakeDispenser : IDispenser {
  int last{0};
  DispenseStats dispenseCoins(int coins) override {
    last = coins; return DispenseStats{true, coins, coins, 0,0,"",0};
  }
};

TEST(Firewall, BindLocalhostOnly) {
  std::filesystem::remove_all("data");
  FakeShutter sh; FakeDispenser disp; TxnConfig cfg; TxnEngine eng(sh, disp, cfg);
  HttpServer srv(eng, sh, disp); ASSERT_TRUE(srv.start("127.0.0.1", 0));
  httplib::Client good("127.0.0.1", srv.port());
  auto res = good.Get("/status");
  EXPECT_TRUE(res);
  httplib::Client bad("127.0.0.2", srv.port());
  auto res2 = bad.Get("/status");
  EXPECT_FALSE(res2);
  srv.stop();
}
