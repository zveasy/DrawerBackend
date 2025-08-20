#include <gtest/gtest.h>
#include <filesystem>
#include "../src/server/http_server.hpp"
#include <httplib.h>
#include "ssl_helpers.hpp"
#include <cstdlib>

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
  FakeShutter sh;
  FakeDispenser disp;
  TxnConfig cfg;
  TxnEngine eng(sh, disp, cfg);
  std::string cert, key;
  if (tls) write_test_cert(std::filesystem::temp_directory_path() / "httptest", cert, key);
  HttpServer srv(eng, sh, disp);
  ASSERT_TRUE(tls ? srv.start("127.0.0.1", 0, cert, key) : srv.start("127.0.0.1", 0));
  if (tls) {
    httplib::SSLClient cli("127.0.0.1", srv.port());
    cli.enable_server_certificate_verification(false);
    auto res = cli.Post("/txn", "{\"price\":735,\"deposit\":1000}", "application/json");
    ASSERT_TRUE(res);
    EXPECT_EQ(200, res->status);
    EXPECT_NE(std::string::npos, res->body.find("\"quarters\":10"));
    EXPECT_NE(std::string::npos, res->body.find("\"status\":\"OK\""));
    auto res2 = cli.Get("/status");
    ASSERT_TRUE(res2);
    EXPECT_EQ(200, res2->status);
    EXPECT_NE(std::string::npos, res2->body.find("\"in_progress\":false"));
    EXPECT_NE(std::string::npos, res2->body.find("\"change\":265"));
    auto res3 = cli.Post("/command", "{\"dispense\":2}", "application/json");
    ASSERT_TRUE(res3);
    EXPECT_EQ(200, res3->status);
    EXPECT_NE(std::string::npos, res3->body.find("\"ok\":true"));
  } else {
    httplib::Client cli("127.0.0.1", srv.port());
    auto res = cli.Post("/txn", "{\"price\":735,\"deposit\":1000}", "application/json");
    ASSERT_TRUE(res);
    EXPECT_EQ(200, res->status);
    EXPECT_NE(std::string::npos, res->body.find("\"quarters\":10"));
    EXPECT_NE(std::string::npos, res->body.find("\"status\":\"OK\""));
    auto res2 = cli.Get("/status");
    ASSERT_TRUE(res2);
    EXPECT_EQ(200, res2->status);
    EXPECT_NE(std::string::npos, res2->body.find("\"in_progress\":false"));
    EXPECT_NE(std::string::npos, res2->body.find("\"change\":265"));
    auto res3 = cli.Post("/command", "{\"dispense\":2}", "application/json");
    ASSERT_TRUE(res3);
    EXPECT_EQ(200, res3->status);
    EXPECT_NE(std::string::npos, res3->body.find("\"ok\":true"));
  }

  srv.stop();
}

TEST_P(HttpHandlers, RateLimit) {
  bool tls = GetParam();
  std::filesystem::remove_all("data");
  FakeShutter sh;
  FakeDispenser disp;
  TxnConfig cfg;
  TxnEngine eng(sh, disp, cfg);
  std::string cert, key;
  if (tls) write_test_cert(std::filesystem::temp_directory_path() / "httptest_rl", cert, key);
  HttpServer srv(eng, sh, disp);
  ASSERT_TRUE(tls ? srv.start("127.0.0.1", 0, cert, key) : srv.start("127.0.0.1", 0));
  if (tls) {
    httplib::SSLClient cli("127.0.0.1", srv.port());
    cli.enable_server_certificate_verification(false);
    httplib::Result r;
    for (int i = 0; i < 10; ++i) {
      r = cli.Post("/txn", "{\"price\":1,\"deposit\":1}", "application/json");
      ASSERT_TRUE(r);
    }
    EXPECT_EQ(429, r->status);
    for (int i = 0; i < 10; ++i) {
      r = cli.Post("/command", "{\"dispense\":1}", "application/json");
      ASSERT_TRUE(r);
    }
    EXPECT_EQ(429, r->status);
  } else {
    httplib::Client cli("127.0.0.1", srv.port());
    httplib::Result r;
    for (int i = 0; i < 10; ++i) {
      r = cli.Post("/txn", "{\"price\":1,\"deposit\":1}", "application/json");
      ASSERT_TRUE(r);
    }
    EXPECT_EQ(429, r->status);
    for (int i = 0; i < 10; ++i) {
      r = cli.Post("/command", "{\"dispense\":1}", "application/json");
      ASSERT_TRUE(r);
    }
    EXPECT_EQ(429, r->status);
  }
  srv.stop();
}

TEST_P(HttpHandlers, MalformedJson) {
  bool tls = GetParam();
  std::filesystem::remove_all("data");
  FakeShutter sh;
  FakeDispenser disp;
  TxnConfig cfg;
  TxnEngine eng(sh, disp, cfg);
  std::string cert, key;
  if (tls) write_test_cert(std::filesystem::temp_directory_path() / "httptest2", cert, key);
  HttpServer srv(eng, sh, disp);
  ASSERT_TRUE(tls ? srv.start("127.0.0.1", 0, cert, key) : srv.start("127.0.0.1", 0));
  setenv("LOG_JSON", "1", 1);
  if (tls) {
    httplib::SSLClient cli("127.0.0.1", srv.port());
    cli.enable_server_certificate_verification(false);
    testing::internal::CaptureStdout();
    auto bad_txn = cli.Post("/txn", "not json", "application/json");
    std::string log_txn = testing::internal::GetCapturedStdout();
    ASSERT_TRUE(bad_txn);
    EXPECT_EQ(400, bad_txn->status);
    EXPECT_NE(log_txn.find("\"route\":\"/txn\""), std::string::npos);
    EXPECT_NE(log_txn.find("\"reason\":\"bad_json\""), std::string::npos);

    testing::internal::CaptureStdout();
    auto bad_cmd = cli.Post("/command", "{\"open\":\"bad\"}", "application/json");
    std::string log_cmd = testing::internal::GetCapturedStdout();
    ASSERT_TRUE(bad_cmd);
    EXPECT_EQ(400, bad_cmd->status);
    EXPECT_NE(log_cmd.find("\"route\":\"/command\""), std::string::npos);
    EXPECT_NE(log_cmd.find("\"reason\":\"bad_request\""), std::string::npos);

    testing::internal::CaptureStdout();
    auto bad_cmd2 = cli.Post("/command", "not json", "application/json");
    std::string log_cmd2 = testing::internal::GetCapturedStdout();
    ASSERT_TRUE(bad_cmd2);
    EXPECT_EQ(400, bad_cmd2->status);
    EXPECT_NE(log_cmd2.find("\"route\":\"/command\""), std::string::npos);
    EXPECT_NE(log_cmd2.find("\"reason\":\"bad_json\""), std::string::npos);
  } else {
    httplib::Client cli("127.0.0.1", srv.port());
    testing::internal::CaptureStdout();
    auto bad_txn = cli.Post("/txn", "not json", "application/json");
    std::string log_txn = testing::internal::GetCapturedStdout();
    ASSERT_TRUE(bad_txn);
    EXPECT_EQ(400, bad_txn->status);
    EXPECT_NE(log_txn.find("\"route\":\"/txn\""), std::string::npos);
    EXPECT_NE(log_txn.find("\"reason\":\"bad_json\""), std::string::npos);

    testing::internal::CaptureStdout();
    auto bad_cmd = cli.Post("/command", "{\"open\":\"bad\"}", "application/json");
    std::string log_cmd = testing::internal::GetCapturedStdout();
    ASSERT_TRUE(bad_cmd);
    EXPECT_EQ(400, bad_cmd->status);
    EXPECT_NE(log_cmd.find("\"route\":\"/command\""), std::string::npos);
    EXPECT_NE(log_cmd.find("\"reason\":\"bad_request\""), std::string::npos);

    testing::internal::CaptureStdout();
    auto bad_cmd2 = cli.Post("/command", "not json", "application/json");
    std::string log_cmd2 = testing::internal::GetCapturedStdout();
    ASSERT_TRUE(bad_cmd2);
    EXPECT_EQ(400, bad_cmd2->status);
    EXPECT_NE(log_cmd2.find("\"route\":\"/command\""), std::string::npos);
    EXPECT_NE(log_cmd2.find("\"reason\":\"bad_json\""), std::string::npos);
  }
  setenv("LOG_JSON", "0", 1);

  srv.stop();
}

TEST_P(HttpHandlers, MetricsRequireAuth) {
  bool tls = GetParam();
  std::filesystem::remove_all("data");
  FakeShutter sh;
  FakeDispenser disp;
  TxnConfig cfg;
  TxnEngine eng(sh, disp, cfg);
  std::string cert, key;
  if (tls) write_test_cert(std::filesystem::temp_directory_path() / "httptest3", cert, key);
  HttpServer srv(eng, sh, disp);
  ASSERT_TRUE(tls ? srv.start("127.0.0.1", 0, cert, key, "tok")
                  : srv.start("127.0.0.1", 0, "", "", "tok"));
  if (tls) {
    httplib::SSLClient cli("127.0.0.1", srv.port());
    cli.enable_server_certificate_verification(false);
    auto res = cli.Get("/metrics");
    ASSERT_TRUE(res);
    EXPECT_EQ(401, res->status);
    auto resj = cli.Get("/metrics.json");
    ASSERT_TRUE(resj);
    EXPECT_EQ(401, resj->status);
  } else {
    httplib::Client cli("127.0.0.1", srv.port());
    auto res = cli.Get("/metrics");
    ASSERT_TRUE(res);
    EXPECT_EQ(401, res->status);
    auto resj = cli.Get("/metrics.json");
    ASSERT_TRUE(resj);
    EXPECT_EQ(401, resj->status);
  }
  srv.stop();
}

INSTANTIATE_TEST_SUITE_P(HttpAndHttps, HttpHandlers, ::testing::Values(false, true));
