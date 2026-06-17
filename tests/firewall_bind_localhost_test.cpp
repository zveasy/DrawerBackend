#include <gtest/gtest.h>
#include "../src/server/http_server.hpp"
#include <httplib.h>
#include <filesystem>
#include "ssl_helpers.hpp"

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

class Firewall : public ::testing::TestWithParam<bool> {};

TEST_P(Firewall, BindLocalhostOnly) {
  bool tls = GetParam();
  std::filesystem::remove_all("data");
  FakeShutter sh; FakeDispenser disp; TxnConfig cfg; TxnEngine eng(sh, disp, cfg);
  std::string cert,key; if (tls) write_test_cert(std::filesystem::temp_directory_path()/"httptest5", cert, key);
  HttpServer srv(eng, sh, disp); ASSERT_TRUE(tls ? srv.start("127.0.0.1", 0, cert, key) : srv.start("127.0.0.1", 0));
  if (tls) {
    httplib::SSLClient good("127.0.0.1", srv.port());
    good.enable_server_certificate_verification(false);
    good.set_connection_timeout(0, 200000);
    good.set_read_timeout(0, 200000);
    auto res = good.Get("/status");
    EXPECT_TRUE(res);
  } else {
    httplib::Client good("127.0.0.1", srv.port());
    good.set_connection_timeout(0, 200000);
    good.set_read_timeout(0, 200000);
    auto res = good.Get("/status");
    EXPECT_TRUE(res);
  }
  srv.stop();
}

INSTANTIATE_TEST_SUITE_P(HttpAndHttps, Firewall, ::testing::Values(false, true));
