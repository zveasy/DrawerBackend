#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include "../src/server/http_server.hpp"
#include <httplib.h>

struct DummyShutter : IShutter {
  bool home(int, std::string*) override { return true; }
  bool open_mm(int, std::string*) override { return true; }
  bool close_mm(int, std::string*) override { return true; }
};

struct DummyDisp : IDispenser {
  DispenseStats dispenseCoins(int coins) override {
    return DispenseStats{true, coins, coins, 0, 0, "", 0};
  }
};

TEST(HelpEndpoint, ServesDocs) {
  namespace fs = std::filesystem;
  fs::path root = fs::temp_directory_path()/"reg_docs";
  fs::create_directories(root/"operator");
  std::ofstream(root/"operator/QuickStart.md") << "Quick start content";
  setenv("REGISTER_MVP_DOCS_PATH", root.c_str(), 1);
  DummyShutter sh; DummyDisp disp; TxnConfig cfg; TxnEngine eng(sh, disp, cfg);
  HttpServer srv(eng, sh, disp); ASSERT_TRUE(srv.start("127.0.0.1",0));
  httplib::Client cli("127.0.0.1", srv.port());
  auto res = cli.Get("/help");
  ASSERT_TRUE(res); EXPECT_EQ(200,res->status);
  EXPECT_NE(std::string::npos, res->body.find("operator"));
  auto res2 = cli.Get("/help/operator/QuickStart.md");
  ASSERT_TRUE(res2); EXPECT_EQ(200,res2->status);
  EXPECT_NE(std::string::npos, res2->body.find("Quick start content"));
  srv.stop();
}

