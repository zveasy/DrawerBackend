#include <gtest/gtest.h>
#include <filesystem>
#include "../src/pos/http_pos.hpp"
#include <httplib.h>
#include <thread>

struct FakeShutter : IShutter {
  bool home(int, std::string*) override { return true; }
  bool open_mm(int, std::string*) override { return true; }
  bool close_mm(int, std::string*) override { return true; }
};

struct FakeDisp : IDispenser {
  DispenseStats dispenseCoins(int c) override {
    return DispenseStats{true, c, c, 0, 0, "", 0};
  }
};

TEST(PosContract, PurchaseAndBusy) {
  std::filesystem::remove_all("data");
  FakeShutter sh; FakeDisp disp; TxnConfig cfg; TxnEngine eng(sh, disp, cfg);
  pos::Options opt; opt.port = 0; opt.bind = "127.0.0.1";
  pos::HttpConnector conn(eng, opt); ASSERT_TRUE(conn.start());
  int port = conn.port();
  httplib::Client cli("127.0.0.1", port);
  auto res = cli.Post("/purchase", "{\"price\":735,\"deposit\":1000}", "application/json");
  ASSERT_TRUE(res);
  EXPECT_EQ(200, res->status);
  EXPECT_NE(std::string::npos, res->body.find("\"change\":265"));
  EXPECT_NE(std::string::npos, res->body.find("\"quarters\":10"));
  EXPECT_NE(std::string::npos, res->body.find("\"status\":\"OK\""));

  auto send = [&](int& st){
    httplib::Client c("127.0.0.1", port);
    auto r = c.Post("/purchase", "{\"price\":100,\"deposit\":200}", "application/json");
    st = r ? r->status : 0;
  };
  int s1=0,s2=0; std::thread t1(send,std::ref(s1)); std::thread t2(send,std::ref(s2));
  t1.join(); t2.join();
  int ok = (s1==200)+(s2==200); int busy=(s1==409)+(s2==409);
  EXPECT_EQ(1, ok); EXPECT_EQ(1,busy);

  conn.stop();
}
