#include "eol/eol_runner.hpp"
#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include <sstream>

struct ReportMockShutter : IShutter {
  bool home(int, std::string*) override { return true; }
  bool open_mm(int, std::string*) override { return true; }
  bool close_mm(int, std::string*) override { return true; }
};

struct ReportMockDisp : IDispenser {
  DispenseStats dispenseCoins(int coins) override {
    return {true, coins, coins, 0, 0, "", 0};
  }
};

struct ReportMockScale : IScale {
  bool first{true};
  long read_average(int) override {
    if (first) {
      first = false;
      return 100;
    }
    return 17110;
  }
};

TEST(EolRunner, ResultJsonContainsSchemaFields) {
  auto lr = cfg::load();
  ASSERT_TRUE(lr.errors.empty());
  cfg::Config cfg = lr.config;
  cfg.eol.result_dir = "eol_schema_tmp";
  std::filesystem::remove_all(cfg.eol.result_dir);

  ReportMockShutter sh;
  ReportMockDisp disp;
  ReportMockScale sc;

  std::ofstream("telemetry.log") << "status=OK";
  eol::Runner runner(sh, disp, &sc, "telemetry.log");
  auto result = runner.run_once(cfg);
  ASSERT_TRUE(std::filesystem::exists(result.report_path));

  std::ifstream in(result.report_path);
  std::stringstream buf;
  buf << in.rdbuf();
  const std::string json = buf.str();

  EXPECT_NE(json.find("\"serial\""), std::string::npos);
  EXPECT_NE(json.find("\"device_id\""), std::string::npos);
  EXPECT_NE(json.find("\"version\""), std::string::npos);
  EXPECT_NE(json.find("\"pass\""), std::string::npos);
  EXPECT_NE(json.find("\"dispensed\""), std::string::npos);
  EXPECT_NE(json.find("\"expected_g\""), std::string::npos);
  EXPECT_NE(json.find("\"measured_g\""), std::string::npos);
  EXPECT_NE(json.find("\"delta_g\""), std::string::npos);
  EXPECT_NE(json.find("\"steps\""), std::string::npos);
  EXPECT_NE(json.find("\"name\":\"Self-check\""), std::string::npos);
  EXPECT_NE(json.find("\"name\":\"Telemetry seen\""), std::string::npos);

  std::filesystem::remove_all(cfg.eol.result_dir);
  std::remove("telemetry.log");
}
