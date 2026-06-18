#include <gtest/gtest.h>

#include <algorithm>
#include <filesystem>
#include <thread>

#include <httplib.h>

#include "cloud/fleet_control/control_plane.hpp"
#include "cloud/fleet_control/routes.hpp"

using namespace cloud::fleet_control;

namespace {

DeviceRegistryRecord device(std::string id, std::string org, std::string merchant, std::string branch,
                            std::string region) {
  DeviceRegistryRecord d;
  d.device_id = std::move(id);
  d.organization_id = std::move(org);
  d.merchant_id = std::move(merchant);
  d.branch_id = std::move(branch);
  d.region = std::move(region);
  d.environment = "pilot";
  d.deployment_channel = "pilot";
  d.firmware_version = "1.0.0";
  d.enrollment_status = "enrolled";
  d.certificate_identity_status = "active";
  d.health_status = "healthy";
  d.tags = {"agent-banking", "cash-heavy"};
  d.labels = {{"country", "KE"}, {"tier", "pilot"}};
  return d;
}

struct RunningServer {
  httplib::Server server;
  std::thread thread;
  int port{0};
  ~RunningServer() {
    server.stop();
    if (thread.joinable()) thread.join();
  }
  void start() {
    port = server.bind_to_any_port("127.0.0.1");
    ASSERT_GT(port, 0);
    thread = std::thread([this] { server.listen_after_bind(); });
    server.wait_until_ready();
  }
};

bool bearer_auth(const httplib::Request& req, httplib::Response& res) {
  if (req.get_header_value("Authorization") == "Bearer token") return true;
  res.status = 401;
  res.set_content("{\"error\":\"unauthorized\"}", "application/json");
  return false;
}

}  // namespace

TEST(FleetControlPlane, HierarchySearchAndPersistence) {
  auto path = std::filesystem::temp_directory_path() / "fleet_control_store.json";
  std::filesystem::remove(path);
  {
    FleetControlPlane cp(path.string(), 30);
    cp.upsert_organization({"org-1", "Org", {"bank"}, {{"market", "east-africa"}}});
    cp.upsert_region({"reg-1", "org-1", "Nairobi", {}, {}});
    cp.upsert_merchant({"merch-1", "org-1", "Merchant", {}, {}});
    cp.upsert_branch({"branch-1", "merch-1", "org-1", "reg-1", "CBD", {}, {}});
    EXPECT_TRUE(cp.register_device(device("dev-1", "org-1", "merch-1", "branch-1", "KE-NBO")));
    EXPECT_TRUE(cp.register_device(device("dev-2", "org-1", "merch-1", "branch-1", "KE-NBO")));
  }

  FleetControlPlane restarted(path.string(), 30);
  EXPECT_EQ(1u, restarted.organizations("org-1").size());
  DeviceFilter filter;
  filter.organization_id = "org-1";
  filter.tag = "agent-banking";
  filter.label_key = "country";
  filter.label_value = "KE";
  filter.text = "dev-";
  EXPECT_EQ(2u, restarted.search_devices(filter).size());
}

TEST(FleetControlPlane, HeartbeatTransitionsCreateEventsAndAlerts) {
  FleetControlPlane cp("", 10);
  cp.register_device(device("dev-1", "org-1", "merch-1", "branch-1", "KE-NBO"));

  EXPECT_TRUE(cp.receive_heartbeat({"dev-1", 95, 100, 5.0}));
  auto online = cp.device("dev-1");
  ASSERT_TRUE(online);
  EXPECT_EQ("online", online->connectivity_status);

  EXPECT_EQ(1, cp.detect_stale_devices(120));
  auto stale = cp.device("dev-1");
  ASSERT_TRUE(stale);
  EXPECT_EQ("offline", stale->connectivity_status);
  EXPECT_FALSE(cp.alerts("org-1").empty());
  auto events = cp.events("org-1");
  EXPECT_NE(events.end(), std::find_if(events.begin(), events.end(), [](const auto& e) {
              return e.type == "heartbeat_received";
            }));
  EXPECT_NE(events.end(), std::find_if(events.begin(), events.end(), [](const auto& e) {
              return e.type == "device_offline";
            }));
}

TEST(FleetControlPlane, CommandQueueTracksDeliveryAckRetriesAndReplayProtection) {
  FleetControlPlane cp("", 30);
  cp.register_device(device("dev-1", "org-1", "merch-1", "branch-1", "KE-NBO"));

  auto cmd = cp.create_command("org-1", "dev-1", "restart", 100, 20, 1);
  ASSERT_EQ("pending", cmd.status);
  auto delivered = cp.deliver_next_command("dev-1", 101);
  ASSERT_TRUE(delivered);
  EXPECT_EQ("delivered", delivered->status);
  auto retry = cp.deliver_next_command("dev-1", 102);
  ASSERT_TRUE(retry);
  EXPECT_EQ(1, retry->retry_count);
  EXPECT_FALSE(cp.deliver_next_command("dev-1", 103));  // max retry marks failed

  auto failed = cp.commands("org-1", "dev-1");
  ASSERT_EQ(1u, failed.size());
  EXPECT_EQ("failed", failed.front().status);

  auto ack_cmd = cp.create_command("org-1", "dev-1", "refresh_config", 110, 20);
  EXPECT_TRUE(cp.acknowledge_command("dev-1", ack_cmd.command_id, 111));
  EXPECT_FALSE(cp.acknowledge_command("dev-1", ack_cmd.command_id, 112));

  auto expiring = cp.create_command("org-1", "dev-1", "sync_inventory", 200, 5);
  EXPECT_EQ(1, cp.expire_commands(206));
  auto commands = cp.commands("org-1", "dev-1");
  EXPECT_NE(commands.end(), std::find_if(commands.begin(), commands.end(), [&](const auto& c) {
              return c.command_id == expiring.command_id && c.status == "expired";
            }));
}

TEST(FleetControlPlane, MetricsAndDashboardAggregateFleetHealth) {
  FleetControlPlane cp("", 30);
  auto d1 = device("dev-1", "org-1", "merch-1", "branch-1", "KE-NBO");
  auto d2 = device("dev-2", "org-1", "merch-1", "branch-1", "KE-NBO");
  d2.firmware_version = "1.1.0";
  d2.health_score = 50;
  d2.sync_failures = 2;
  d2.ota_failures = 1;
  d2.certificate_identity_status = "expired";
  cp.register_device(d1);
  cp.register_device(d2);
  cp.receive_heartbeat({"dev-1", 100, 110, 10.0});
  cp.create_alert("org-1", "dev-2", "certificate_expiration", "warning", "certificate near expiry", 111);

  auto metrics = cp.metrics("org-1");
  EXPECT_EQ(1, metrics.online_devices);
  EXPECT_EQ(1, metrics.offline_devices);
  EXPECT_DOUBLE_EQ(0.5, metrics.fleet_availability);
  EXPECT_EQ(1, metrics.alert_counts["warning"]);

  auto dash = cp.dashboard("organization", "org-1");
  EXPECT_EQ(2, dash.device_count);
  EXPECT_EQ(1, dash.online_devices);
  EXPECT_EQ(1, dash.firmware_distribution["1.0.0"]);
  EXPECT_LT(dash.health.aggregate_score, 100);
}

TEST(FleetControlPlaneApi, RequiresAuthAndIsolatesTenants) {
  FleetControlPlane cp("", 30);
  cp.register_device(device("dev-1", "org-1", "m1", "b1", "KE-NBO"));
  cp.register_device(device("dev-2", "org-2", "m2", "b2", "NG-LOS"));
  RunningServer srv;
  register_fleet_control_routes(srv.server, cp, bearer_auth);
  srv.start();

  httplib::Client cli("127.0.0.1", srv.port);
  auto unauthorized = cli.Get("/fleet-control/v1/devices");
  ASSERT_TRUE(unauthorized);
  EXPECT_EQ(401, unauthorized->status);

  httplib::Headers org1{{"Authorization", "Bearer token"}, {"X-Org-Id", "org-1"}};
  auto res = cli.Get("/fleet-control/v1/devices", org1);
  ASSERT_TRUE(res);
  EXPECT_EQ(200, res->status);
  auto body = nlohmann::json::parse(res->body);
  ASSERT_EQ(1u, body["devices"].size());
  EXPECT_EQ("dev-1", body["devices"][0]["device_id"]);

  auto forbidden = cli.Post("/fleet-control/v1/devices", org1,
                            nlohmann::json(device("dev-3", "org-2", "m2", "b2", "NG-LOS")).dump(),
                            "application/json");
  ASSERT_TRUE(forbidden);
  EXPECT_EQ(403, forbidden->status);
}
