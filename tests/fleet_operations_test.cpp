#include <gtest/gtest.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <thread>

#include <httplib.h>
#include <nlohmann/json.hpp>

#include "cloud/fleet_operations/routes.hpp"
#include "integrations/veil/client.hpp"

using namespace cloud::fleet_operations;

namespace {

FleetDevice device(std::string id = "dev-1", std::string tenant = "org-1") {
  FleetDevice d;
  d.device_id = std::move(id);
  d.tenant_id = std::move(tenant);
  d.merchant_id = "merchant-1";
  d.branch_id = "branch-1";
  d.location_id = "location-1";
  d.fleet_id = "fleet-1";
  d.group_id = "group-1";
  d.region = "KE-NBO";
  d.device_type = "cash_drawer";
  d.firmware_version = "1.0.0";
  d.driver_id = "mock-cash_drawer";
  d.lifecycle_state = "active";
  d.capabilities = {"inventory", "open_close", "lock_control", "sensor_telemetry",
                    "command_execution", "identity", "evidence_export"};
  return d;
}

FleetHeartbeat heartbeat(std::string id = "dev-1", std::string tenant = "org-1",
                         long timestamp = 100) {
  FleetHeartbeat hb;
  hb.device_id = std::move(id);
  hb.tenant_id = std::move(tenant);
  hb.firmware_version = "1.0.1";
  hb.driver_version = "mock-1";
  hb.connectivity_status = "online";
  hb.inventory_summary = {{"drift", 0.0}, {"contradiction", false}};
  hb.health_metrics = {{"self_test_passed", true}, {"driver_stability", 1.0}};
  hb.timestamp = timestamp;
  hb.signature = "test-signature";
  return hb;
}

FleetCommand command(std::string type, long expires_at = 200) {
  FleetCommand cmd;
  cmd.tenant_id = "org-1";
  cmd.device_id = "dev-1";
  cmd.command = std::move(type);
  cmd.actor_id = "operator-1";
  cmd.expires_at = expires_at;
  return cmd;
}

struct Fixture {
  integrations::veil::LocalVeilClient client;
  integrations::veil::VeilTrustService trust;
  edge_platform::EdgePlatformService edge;
  FleetOperationsService service;

  explicit Fixture(std::string path = "")
      : trust("", &client), edge("", nullptr, &trust),
        service(std::move(path), &edge, &trust, 30) {}

  void enroll_and_heartbeat() {
    ASSERT_TRUE(service.enroll(device(), "operator-1", 90));
    ASSERT_TRUE(service.receive_heartbeat(heartbeat(), 101));
  }
};

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

TEST(FleetOperations, EnrollmentLifecyclePersistenceAndIrreversibleRetirement) {
  auto path = std::filesystem::temp_directory_path() / "fleet_operations_lifecycle.json";
  std::filesystem::remove(path);
  {
    Fixture f(path.string());
    auto d = device();
    d.lifecycle_state = "provisioned";
    ASSERT_TRUE(f.service.enroll(d, "provisioner", 10));
    EXPECT_TRUE(f.service.update_device("org-1", "dev-1", {{"lifecycle_state", "active"}}, 20));
    EXPECT_TRUE(f.service.update_device("org-1", "dev-1", {{"lifecycle_state", "maintenance"}}, 30));
    EXPECT_FALSE(f.service.update_device("org-1", "dev-1", {{"lifecycle_state", "provisioned"}}, 40));
    EXPECT_TRUE(f.service.retire_device("org-1", "dev-1", 50));
    EXPECT_FALSE(f.service.update_device("org-1", "dev-1", {{"lifecycle_state", "active"}}, 60));
    EXPECT_FALSE(f.service.retire_device("org-1", "dev-1", 70));
  }
  Fixture restarted(path.string());
  auto d = restarted.service.device("org-1", "dev-1");
  ASSERT_TRUE(d);
  EXPECT_TRUE(d->retired);
  EXPECT_EQ("retired", d->lifecycle_state);
  EXPECT_FALSE(restarted.service.device("org-2", "dev-1"));
}

TEST(FleetOperations, HeartbeatsFailClosedForStaleTenantAndUnknownCapabilityState) {
  Fixture f;
  ASSERT_TRUE(f.service.enroll(device(), "operator", 10));

  std::string error;
  EXPECT_FALSE(f.service.receive_heartbeat(heartbeat("dev-1", "org-1", 50), 100, &error));
  EXPECT_EQ("stale_heartbeat", error);
  EXPECT_EQ("quarantined", f.service.device("org-1", "dev-1")->lifecycle_state);

  auto other = device("dev-2");
  ASSERT_TRUE(f.service.enroll(other, "operator", 10));
  EXPECT_FALSE(f.service.receive_heartbeat(heartbeat("dev-2", "org-2", 100), 101, &error));
  EXPECT_EQ("tenant_mismatch", error);

  auto unknown = device("dev-3");
  unknown.capabilities.clear();
  FleetOperationsService isolated("", nullptr, &f.trust, 30);
  EXPECT_FALSE(isolated.enroll(unknown, "operator", 10, &error));
  EXPECT_EQ("unknown_capability_state", error);
}

TEST(FleetOperations, CommandTransitionsCancellationExpirationCapabilityAndPolicy) {
  Fixture f;
  f.enroll_and_heartbeat();

  auto normal = f.service.create_command(command("run_self_test"), 110);
  ASSERT_EQ("dispatched", normal.state);
  auto duplicate = command("run_self_test");
  duplicate.command_id = normal.command_id;
  auto duplicate_result = f.service.create_command(duplicate, 110);
  EXPECT_EQ("failed", duplicate_result.state);
  EXPECT_EQ("duplicate_command_id", duplicate_result.failure_reason);
  EXPECT_EQ("dispatched", f.service.command("org-1", normal.command_id)->state);
  EXPECT_TRUE(f.service.acknowledge_command("org-1", normal.command_id, 111));
  EXPECT_TRUE(f.service.complete_command("org-1", normal.command_id, true, "passed", 112));
  EXPECT_EQ("completed", f.service.command("org-1", normal.command_id)->state);
  EXPECT_FALSE(f.service.acknowledge_command("org-1", normal.command_id, 113));

  auto cancellable = f.service.create_command(command("run_self_test"), 114);
  EXPECT_TRUE(f.service.cancel_command("org-1", cancellable.command_id, 115));
  EXPECT_FALSE(f.service.acknowledge_command("org-1", cancellable.command_id, 116));

  auto expiring = command("run_self_test", 118);
  auto expiring_result = f.service.create_command(expiring, 117);
  EXPECT_EQ("dispatched", expiring_result.state);
  EXPECT_EQ(1, f.service.expire_commands(119));
  EXPECT_EQ("expired", f.service.command("org-1", expiring_result.command_id)->state);

  auto unsupported = f.service.create_command(command("dispense_cash"), 120);
  EXPECT_EQ("failed", unsupported.state);
  EXPECT_EQ("unsupported_capability", unsupported.failure_reason);

  f.client.deny_action("command_execution");
  auto denied = f.service.create_command(command("run_self_test"), 121);
  EXPECT_EQ("failed", denied.state);
  EXPECT_EQ("policy_denied", denied.failure_reason);
  EXPECT_EQ("quarantined", f.service.device("org-1", "dev-1")->lifecycle_state);
}

TEST(FleetOperations, HighRiskCommandsRequireApprovalAndOfflineDevicesAreRejected) {
  Fixture f;
  f.enroll_and_heartbeat();

  auto high_risk = command("unlock");
  auto pending = f.service.create_command(high_risk, 110);
  EXPECT_EQ("policy_checking", pending.state);
  EXPECT_EQ("approval_required", pending.failure_reason);

  high_risk.command_id = "approved-command";
  high_risk.approval_id = "approval-1";
  high_risk.approved_by = "supervisor-1";
  auto approved = f.service.create_command(high_risk, 111);
  EXPECT_EQ("dispatched", approved.state);

  auto offline = heartbeat();
  offline.timestamp = 112;
  offline.connectivity_status = "offline";
  ASSERT_TRUE(f.service.receive_heartbeat(offline, 113));
  auto rejected = f.service.create_command(command("run_self_test"), 114);
  EXPECT_EQ("failed", rejected.state);
  EXPECT_EQ("device_offline", rejected.failure_reason);
}

TEST(FleetOperations, RiskScoringIsDeterministicExplainableAndLocationAware) {
  Fixture f;
  f.enroll_and_heartbeat();
  auto risky = heartbeat();
  risky.timestamp = 105;
  risky.inventory_summary["drift"] = 0.25;
  risky.health_metrics["driver_stability"] = 0.5;
  risky.health_metrics["unusual_cash_movement"] = true;
  risky.health_metrics["missing_trust_evidence"] = true;
  ASSERT_TRUE(f.service.receive_heartbeat(risky, 106));

  auto first = f.service.device_risk("org-1", "dev-1", 110);
  auto second = f.service.device_risk("org-1", "dev-1", 110);
  EXPECT_EQ(first.score, second.score);
  EXPECT_EQ(first.level, second.level);
  EXPECT_EQ("critical", first.level);
  EXPECT_GE(first.factors.size(), 4u);
  auto events = f.service.events("org-1");
  EXPECT_NE(events.end(), std::find_if(events.begin(), events.end(), [](const auto& event) {
              return event.event_type == "risk_changed";
            }));

  auto location = f.service.location_risk("org-1", "location-1", 110);
  EXPECT_EQ(first.score, location.score);
  EXPECT_EQ("location", location.scope_type);
}

TEST(FleetOperations, QuarantineRecoveryRequiresFreshSelfTestAndVeilApproval) {
  Fixture f;
  f.enroll_and_heartbeat();
  ASSERT_TRUE(f.service.quarantine("org-1", "dev-1", "operator_review", 105));

  std::string error;
  EXPECT_FALSE(f.service.recover("org-1", "dev-1", 140, &error));
  EXPECT_EQ("fresh_heartbeat_required", error);

  auto hb = heartbeat("dev-1", "org-1", 141);
  hb.health_metrics["self_test_passed"] = false;
  EXPECT_FALSE(f.service.receive_heartbeat(hb, 142));
  EXPECT_FALSE(f.service.recover("org-1", "dev-1", 143, &error));
  EXPECT_EQ("successful_self_test_required", error);

  hb.timestamp = 144;
  hb.health_metrics["self_test_passed"] = true;
  EXPECT_FALSE(f.service.receive_heartbeat(hb, 145));
  f.client.deny_action("device_recovery");
  EXPECT_FALSE(f.service.recover("org-1", "dev-1", 146, &error));
  EXPECT_EQ("veil_recovery_denied", error);

  f.client.clear_denials();
  EXPECT_TRUE(f.service.recover("org-1", "dev-1", 147, &error));
  EXPECT_EQ("active", f.service.device("org-1", "dev-1")->lifecycle_state);
}

TEST(FleetOperations, EventReplayIsAppendOnlyDeterministicAndSurvivesRestart) {
  auto path = std::filesystem::temp_directory_path() / "fleet_operations_events.json";
  std::filesystem::remove(path);
  std::string hash;
  size_t count = 0;
  {
    Fixture f(path.string());
    f.enroll_and_heartbeat();
    auto cmd = f.service.create_command(command("run_self_test"), 110);
    ASSERT_TRUE(f.service.acknowledge_command("org-1", cmd.command_id, 111));
    ASSERT_TRUE(f.service.complete_command("org-1", cmd.command_id, true, "ok", 112));
    auto replay = f.service.replay("org-1");
    ASSERT_TRUE(replay.valid);
    hash = replay.hash_summary;
    count = replay.events.size();
    ASSERT_GT(count, 4u);
    for (size_t i = 1; i < replay.events.size(); ++i)
      EXPECT_GT(replay.events[i].sequence, replay.events[i - 1].sequence);
  }
  Fixture restarted(path.string());
  auto replay = restarted.service.replay("org-1");
  EXPECT_TRUE(replay.valid);
  EXPECT_EQ(hash, replay.hash_summary);
  EXPECT_EQ(count, replay.events.size());
}

TEST(FleetOperationsApi, RequiresAuthTenantAndEnforcesIsolation) {
  Fixture f;
  RunningServer srv;
  register_fleet_operations_routes(srv.server, f.service, bearer_auth);
  srv.start();

  httplib::Client cli("127.0.0.1", srv.port);
  auto unauthorized = cli.Get("/fleet/v1/devices");
  ASSERT_TRUE(unauthorized);
  EXPECT_EQ(401, unauthorized->status);

  httplib::Headers auth{{"Authorization", "Bearer token"}};
  auto missing_tenant = cli.Get("/fleet/v1/devices", auth);
  ASSERT_TRUE(missing_tenant);
  EXPECT_EQ(400, missing_tenant->status);

  httplib::Headers org1{{"Authorization", "Bearer token"}, {"X-Org-Id", "org-1"}};
  auto enroll_body = nlohmann::json(device());
  enroll_body["actor_id"] = "operator-1";
  enroll_body["now_epoch"] = 10;
  auto enrolled = cli.Post("/fleet/v1/devices/enroll", org1, enroll_body.dump(), "application/json");
  ASSERT_TRUE(enrolled);
  EXPECT_EQ(201, enrolled->status);

  auto listed = cli.Get("/fleet/v1/devices", org1);
  ASSERT_TRUE(listed);
  EXPECT_EQ(200, listed->status);
  EXPECT_EQ(1u, nlohmann::json::parse(listed->body)["devices"].size());

  httplib::Headers org2{{"Authorization", "Bearer token"}, {"X-Org-Id", "org-2"}};
  auto hidden = cli.Get("/fleet/v1/devices/dev-1", org2);
  ASSERT_TRUE(hidden);
  EXPECT_EQ(404, hidden->status);
}
