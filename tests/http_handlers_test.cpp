#include <gtest/gtest.h>
#include <filesystem>
#include "../src/server/http_server.hpp"
#include <httplib.h>
#include "ssl_helpers.hpp"

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

class HttpHandlers : public ::testing::TestWithParam<bool> {};

TEST_P(HttpHandlers, BasicFlow) {
  bool tls = GetParam();
  std::filesystem::remove_all("data");
  FakeShutter sh; FakeDispenser disp; TxnConfig cfg; TxnEngine eng(sh, disp, cfg);
  std::string cert, key; if (tls) write_test_cert(std::filesystem::temp_directory_path()/"httptest", cert, key);
  HttpServer srv(eng, sh, disp); ASSERT_TRUE(tls ? srv.start("127.0.0.1", 0, cert, key) : srv.start("127.0.0.1", 0));
  if (tls) {
    httplib::SSLClient cli("127.0.0.1", srv.port());
    cli.enable_server_certificate_verification(false);
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
  } else {
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
  }

  srv.stop();
}

TEST_P(HttpHandlers, MalformedJson) {
  bool tls = GetParam();
  std::filesystem::remove_all("data");
  FakeShutter sh; FakeDispenser disp; TxnConfig cfg; TxnEngine eng(sh, disp, cfg);
  std::string cert, key; if (tls) write_test_cert(std::filesystem::temp_directory_path()/"httptest2", cert, key);
  HttpServer srv(eng, sh, disp); ASSERT_TRUE(tls ? srv.start("127.0.0.1", 0, cert, key) : srv.start("127.0.0.1", 0));
  if (tls) {
    httplib::SSLClient cli("127.0.0.1", srv.port());
    cli.enable_server_certificate_verification(false);
    auto bad_txn = cli.Post("/txn", "not json", "application/json");
    ASSERT_TRUE(bad_txn); EXPECT_EQ(400, bad_txn->status);
    auto bad_cmd = cli.Post("/command", "{\"open\":\"bad\"}", "application/json");
    ASSERT_TRUE(bad_cmd); EXPECT_EQ(400, bad_cmd->status);
    auto bad_cmd2 = cli.Post("/command", "not json", "application/json");
    ASSERT_TRUE(bad_cmd2); EXPECT_EQ(400, bad_cmd2->status);
  } else {
    httplib::Client cli("127.0.0.1", srv.port());
    auto bad_txn = cli.Post("/txn", "not json", "application/json");
    ASSERT_TRUE(bad_txn); EXPECT_EQ(400, bad_txn->status);
    auto bad_cmd = cli.Post("/command", "{\"open\":\"bad\"}", "application/json");
    ASSERT_TRUE(bad_cmd); EXPECT_EQ(400, bad_cmd->status);
    auto bad_cmd2 = cli.Post("/command", "not json", "application/json");
    ASSERT_TRUE(bad_cmd2); EXPECT_EQ(400, bad_cmd2->status);
  }

  srv.stop();
}

INSTANTIATE_TEST_SUITE_P(HttpAndHttps, HttpHandlers, ::testing::Values(false, true));
