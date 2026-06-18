#include <gtest/gtest.h>

#include <httplib.h>
#include <nlohmann/json.hpp>

#include "../src/server/http_server.hpp"

struct FleetFakeShutter : IShutter {
  bool home(int, std::string*) override { return true; }
  bool open_mm(int, std::string*) override { return true; }
  bool close_mm(int, std::string*) override { return true; }
};

struct FleetFakeDispenser : IDispenser {
  DispenseStats dispenseCoins(int coins) override {
    return DispenseStats{true, coins, coins, 0, 0, "", 0};
  }
};

TEST(FleetApi, ExposesDeviceTwinEndpoints) {
  FleetFakeShutter shutter;
  FleetFakeDispenser dispenser;
  TxnConfig cfg;
  TxnEngine engine(shutter, dispenser, cfg);
  HttpServer server(engine, shutter, dispenser);
  ASSERT_TRUE(server.start("127.0.0.1", 0));

  httplib::Client client("127.0.0.1", server.port());
  auto list = client.Get("/fleet/devices");
  ASSERT_TRUE(list);
  ASSERT_EQ(200, list->status);
  auto list_json = nlohmann::json::parse(list->body);
  ASSERT_FALSE(list_json["devices"].empty());
  std::string drawer_id = list_json["devices"][0]["drawer_id"].get<std::string>();

  auto twin = client.Get(("/fleet/device/" + drawer_id).c_str());
  ASSERT_TRUE(twin);
  ASSERT_EQ(200, twin->status);
  auto twin_json = nlohmann::json::parse(twin->body);
  EXPECT_EQ(drawer_id, twin_json["drawer_id"].get<std::string>());
  EXPECT_TRUE(twin_json.contains("firmware"));
  EXPECT_TRUE(twin_json.contains("maintenance"));
  EXPECT_TRUE(twin_json.contains("inventory_forecast"));

  auto health = client.Get(("/fleet/device/" + drawer_id + "/health").c_str());
  ASSERT_TRUE(health);
  EXPECT_EQ(200, health->status);
  EXPECT_TRUE(nlohmann::json::parse(health->body).contains("alerts"));

  auto inventory = client.Get(("/fleet/device/" + drawer_id + "/inventory").c_str());
  ASSERT_TRUE(inventory);
  EXPECT_EQ(200, inventory->status);
  EXPECT_TRUE(nlohmann::json::parse(inventory->body).contains("forecast"));

  auto history = client.Get(("/fleet/device/" + drawer_id + "/history").c_str());
  ASSERT_TRUE(history);
  EXPECT_EQ(200, history->status);
  EXPECT_TRUE(nlohmann::json::parse(history->body).contains("timeline"));

  auto metrics = client.Get("/fleet/metrics");
  ASSERT_TRUE(metrics);
  EXPECT_EQ(200, metrics->status);
  auto metrics_json = nlohmann::json::parse(metrics->body);
  EXPECT_GE(metrics_json["total_drawers"].get<int>(), 1);
  EXPECT_TRUE(metrics_json.contains("firmware_distribution"));

  auto missing = client.Get("/fleet/device/not-real");
  ASSERT_TRUE(missing);
  EXPECT_EQ(404, missing->status);

  server.stop();
}
