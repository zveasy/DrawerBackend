#include <gtest/gtest.h>

#include <algorithm>
#include <thread>

#include <httplib.h>
#include <nlohmann/json.hpp>

#include "edge_platform/routes.hpp"
#include "integrations/veil/client.hpp"

using namespace edge_platform;

namespace {

EdgeDevice device(std::string id, std::string tenant, std::string type) {
  EdgeDevice d;
  d.device_id = std::move(id);
  d.tenant_id = std::move(tenant);
  d.merchant_id = "merchant-1";
  d.branch_id = "branch-1";
  d.region = "KE-NBO";
  d.device_type = std::move(type);
  d.firmware_version = "1.0.0";
  d.labels = {{"pilot", "true"}};
  return d;
}

EdgeCommandRequest command(std::string device_id, std::string cmd, long now = 100) {
  EdgeCommandRequest req;
  req.device_id = std::move(device_id);
  req.command = std::move(cmd);
  req.actor_id = "operator-1";
  req.requested_at = now;
  return req;
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

TEST(EdgePlatform, RegistersAllDeviceTypesAndPreservesDrawerCompatibility) {
  cloud::fleet_control::FleetControlPlane fleet("", 30);
  integrations::veil::LocalVeilClient veil_client;
  integrations::veil::VeilTrustService trust("", &veil_client, &fleet);
  EdgePlatformService svc("", &fleet, &trust);

  std::vector<std::string> types = {"cash_drawer", "smart_safe", "cash_recycler",
                                    "teller_station", "kiosk", "atm_like_terminal"};
  for (const auto& type : types) {
    ASSERT_TRUE(svc.register_device(device("dev-" + type, "org-1", type))) << type;
  }
  auto drawer = fleet.device("dev-cash_drawer");
  ASSERT_TRUE(drawer);
  EXPECT_EQ("cash_drawer", drawer->device_type);
  EXPECT_NE(drawer->capabilities.end(),
            std::find(drawer->capabilities.begin(), drawer->capabilities.end(), "open_close"));
  EXPECT_EQ(types.size(), svc.devices("org-1").size());
}

TEST(EdgePlatform, CapabilityValidationRejectsUnsupportedCommandsFailClosed) {
  EdgePlatformService svc("");
  auto d = device("safe-1", "org-1", "smart_safe");
  d.capabilities = {"inventory", "cash_in", "lock_control", "command_execution"};
  ASSERT_TRUE(svc.register_device(d));

  auto unsupported = svc.execute_command(command("safe-1", "dispense_cash"));
  EXPECT_FALSE(unsupported.executed);
  EXPECT_EQ("rejected", unsupported.status);
  EXPECT_NE(std::string::npos, unsupported.reason.find("missing_capability"));

  auto unknown = svc.execute_command(command("safe-1", "launch_rocket"));
  EXPECT_FALSE(unknown.executed);
  EXPECT_EQ("unsupported_command", unknown.reason);
  auto events = svc.events("org-1");
  EXPECT_NE(events.end(), std::find_if(events.begin(), events.end(), [](const auto& e) {
              return e.event_type == "unsupported_command_rejected";
            }));
}

TEST(EdgePlatform, MockDriverSimulatorInventoryAndHealth) {
  EdgePlatformService svc("");
  ASSERT_TRUE(svc.register_device(device("recycler-1", "org-1", "cash_recycler")));

  auto result = svc.execute_command(command("recycler-1", "accept_cash", 100));
  EXPECT_TRUE(result.executed);
  EXPECT_EQ("executed", result.status);

  auto inv = svc.inventory("recycler-1");
  ASSERT_FALSE(inv.compartments.empty());
  inv.compartments.push_back({"cassette-a", "cassette", "KES", {{"100", 10}, {"500", 5}}, "ok"});
  inv.updated_at = 110;
  EXPECT_TRUE(svc.update_inventory(inv));
  EXPECT_EQ(2u, svc.inventory("recycler-1").compartments.size());

  auto state = svc.simulate("recycler-1", "fault", {{"fault", "cash_jam"}}, 120);
  EXPECT_FALSE(state.faults.empty());
  auto health = svc.health("recycler-1");
  EXPECT_LT(health.score, 100);
  EXPECT_EQ("cash_recycler", health.device_type);

  svc.simulate("recycler-1", "offline", {}, 130);
  auto offline = svc.execute_command(command("recycler-1", "count_cash", 140));
  EXPECT_FALSE(offline.executed);
  EXPECT_EQ("device_offline", offline.reason);
}

TEST(EdgePlatform, PolicyGatedCommandsGenerateTrustEvidence) {
  integrations::veil::LocalVeilClient client;
  integrations::veil::PolicyConfig policy;
  policy.production_mode = true;
  integrations::veil::VeilTrustService trust("", &client, nullptr, nullptr, policy);
  EdgePlatformService svc("", nullptr, &trust);
  ASSERT_TRUE(svc.register_device(device("atm-1", "org-1", "atm_like_terminal")));

  auto allowed = svc.execute_command(command("atm-1", "dispense_cash", 100));
  EXPECT_TRUE(allowed.executed);
  integrations::veil::EvidenceFilter filter;
  filter.tenant_id = "org-1";
  filter.device_id = "atm-1";
  auto evidence = trust.list_evidence(filter);
  EXPECT_FALSE(evidence.empty());

  client.deny_action("unlock");
  auto denied = svc.execute_command(command("atm-1", "unlock", 110));
  EXPECT_FALSE(denied.executed);
  EXPECT_EQ("policy_denied", denied.reason);
}

TEST(EdgePlatformApi, RequiresAuthAndIsolatesTenants) {
  EdgePlatformService svc("");
  ASSERT_TRUE(svc.register_device(device("dev-1", "org-1", "cash_drawer")));
  ASSERT_TRUE(svc.register_device(device("dev-2", "org-2", "kiosk")));

  RunningServer srv;
  register_edge_routes(srv.server, svc, bearer_auth);
  srv.start();

  httplib::Client cli("127.0.0.1", srv.port);
  auto unauthorized = cli.Get("/edge/v1/devices");
  ASSERT_TRUE(unauthorized);
  EXPECT_EQ(401, unauthorized->status);

  httplib::Headers org1{{"Authorization", "Bearer token"}, {"X-Org-Id", "org-1"}};
  auto devices = cli.Get("/edge/v1/devices", org1);
  ASSERT_TRUE(devices);
  EXPECT_EQ(200, devices->status);
  auto body = nlohmann::json::parse(devices->body);
  ASSERT_EQ(1u, body["devices"].size());
  EXPECT_EQ("dev-1", body["devices"][0]["device_id"]);

  auto forbidden = cli.Get("/edge/v1/devices/dev-2/inventory", org1);
  ASSERT_TRUE(forbidden);
  EXPECT_EQ(403, forbidden->status);

  auto cmd = cli.Post("/edge/v1/devices/dev-1/commands", org1,
                      nlohmann::json(command("dev-1", "open", 100)).dump(), "application/json");
  ASSERT_TRUE(cmd);
  EXPECT_EQ(200, cmd->status);
}
