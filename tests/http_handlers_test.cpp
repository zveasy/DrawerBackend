#include <gtest/gtest.h>
#include <filesystem>
#include "../src/server/http_server.hpp"
#include <httplib.h>

struct FakeShutter : IShutter {
  bool home(int, std::string*) override { return true; }
  bool open_mm(int, std::string*) override { return true; }
  bool close_mm(int, std::string*) override { return true; }
};

struct FakeDispenser : IDispenser {
  int last{0};
  DispenseStats dispenseCoins(int coins) override {
    last = coins;
    return DispenseStats{true, coins, coins, 0, 0, "", 0};
  }
};

TEST(HttpHandlers, BasicFlow) {
  std::filesystem::remove_all("data");
  FakeShutter sh; FakeDispenser disp; TxnConfig cfg; TxnEngine eng(sh, disp, cfg);
  HttpServer srv(eng, sh, disp); ASSERT_TRUE(srv.start("127.0.0.1", 0));
  httplib::Client cli("127.0.0.1", srv.port());

  auto res = cli.Post("/txn", "{\"price\":735,\"deposit\":1000}", "application/json");
  ASSERT_TRUE(res); EXPECT_EQ(200, res->status);
  EXPECT_NE(std::string::npos, res->body.find("\"quarters\":10"));
  EXPECT_NE(std::string::npos, res->body.find("\"status\":\"OK\""));

  auto res2 = cli.Get("/status");
  ASSERT_TRUE(res2); EXPECT_EQ(200, res2->status);
  EXPECT_NE(std::string::npos, res2->body.find("\"in_progress\":false"));
  EXPECT_NE(std::string::npos, res2->body.find("\"change\":265"));

  auto res3 = cli.Post("/command", "{\"dispense\":2}", "application/json");
  ASSERT_TRUE(res3); EXPECT_EQ(200, res3->status);
  EXPECT_NE(std::string::npos, res3->body.find("\"ok\":true"));

  srv.stop();
}
