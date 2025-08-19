#include <gtest/gtest.h>
#include <filesystem>
#include "../src/server/http_server.hpp"
#include <httplib.h>
#include <thread>
#include <chrono>
#include "ssl_helpers.hpp"

struct FakeShutter : IShutter {
  bool home(int, std::string*) override { return true; }
  bool open_mm(int, std::string*) override { return true; }
  bool close_mm(int, std::string*) override { return true; }
};

struct SlowDispenser : IDispenser {
  DispenseStats dispenseCoins(int coins) override {
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    return DispenseStats{true, coins, coins, 0, 0, "", 0};
  }
};

class HttpConcurrency : public ::testing::TestWithParam<bool> {};

TEST_P(HttpConcurrency, Busy) {
  bool tls = GetParam();
  std::filesystem::remove_all("data");
  FakeShutter sh; SlowDispenser disp; TxnConfig cfg; TxnEngine eng(sh, disp, cfg);
  std::string cert, key; if (tls) write_test_cert(std::filesystem::temp_directory_path()/"httptest3", cert, key);
  HttpServer srv(eng, sh, disp); ASSERT_TRUE(tls ? srv.start("127.0.0.1", 0, cert, key) : srv.start("127.0.0.1", 0));

  auto send = [&](int& st, std::string& body) {
    if (tls) {
      httplib::SSLClient cli("127.0.0.1", srv.port());
      cli.enable_server_certificate_verification(false);
      if (auto res = cli.Post("/txn", "{\"price\":100,\"deposit\":200}", "application/json")) {
        st = res->status; body = res->body;
      } else st = 0;
    } else {
      httplib::Client cli("127.0.0.1", srv.port());
      if (auto res = cli.Post("/txn", "{\"price\":100,\"deposit\":200}", "application/json")) {
        st = res->status; body = res->body;
      } else st = 0;
    }
  };
  int s1=0,s2=0; std::string b1,b2; std::thread t1(send,std::ref(s1),std::ref(b1)); std::thread t2(send,std::ref(s2),std::ref(b2));
  t1.join(); t2.join();
  int ok = (s1==200) + (s2==200); int busy = (s1==409) + (s2==409);
  EXPECT_EQ(1, ok); EXPECT_EQ(1, busy);
  if (s1==409) EXPECT_NE(std::string::npos, b1.find("busy"));
  if (s2==409) EXPECT_NE(std::string::npos, b2.find("busy"));
  srv.stop();
}

INSTANTIATE_TEST_SUITE_P(HttpAndHttps, HttpConcurrency, ::testing::Values(false, true));
