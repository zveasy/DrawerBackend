#include <gtest/gtest.h>

#include <algorithm>
#include <filesystem>
#include <thread>

#include <httplib.h>

#include "cloud/cash_intelligence/routes.hpp"
#include "cloud/fleet_control/control_plane.hpp"

using namespace cloud::cash_intelligence;

namespace {

CashLedgerEntry entry(std::string event_id, std::string type, long expected, long observed,
                      long occurred_at = 36000) {
  CashLedgerEntry e;
  e.event_type = std::move(type);
  e.event_id = std::move(event_id);
  e.device_id = "dev-1";
  e.merchant_id = "merchant-1";
  e.branch_id = "branch-1";
  e.region = "KE-NBO";
  e.currency = "KES";
  e.denominations = {{"100", 10}, {"500", 2}};
  e.expected_balance = expected;
  e.observed_balance = observed;
  e.actor_id = "operator-1";
  e.actor_role = "cashier";
  e.occurred_at_epoch = occurred_at;
  return e;
}

cloud::fleet_control::DeviceRegistryRecord device(std::string id, std::string org) {
  cloud::fleet_control::DeviceRegistryRecord d;
  d.device_id = std::move(id);
  d.organization_id = std::move(org);
  d.merchant_id = "merchant-1";
  d.branch_id = "branch-1";
  d.region = "KE-NBO";
  d.environment = "pilot";
  d.deployment_channel = "pilot";
  d.firmware_version = "1.0.0";
  d.enrollment_status = "enrolled";
  d.certificate_identity_status = "active";
  d.health_status = "healthy";
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

TEST(CashIntelligence, LedgerPersistenceAndDenominationMath) {
  auto path = std::filesystem::temp_directory_path() / "cash_intelligence_store.json";
  std::filesystem::remove(path);
  {
    CashIntelligenceService svc(path.string());
    svc.configure_denominations("KES", {50, 100, 500, 1000});
    EXPECT_TRUE(svc.add_ledger_entry(entry("txn-1", "transaction", 2000, 2000)));
    EXPECT_EQ(2000, svc.denomination_value({{"100", 10}, {"500", 2}}));
    EXPECT_EQ(2, svc.denomination_drift("KES", {{"100", 10}}, {{"100", 8}}));
    auto bad = entry("bad-denomination", "transaction", 0, 0);
    bad.denominations = {{"7", 1}};
    EXPECT_FALSE(svc.add_ledger_entry(bad));
  }

  CashIntelligenceService restarted(path.string());
  CashFilter filter;
  filter.device_id = "dev-1";
  filter.currency = "KES";
  EXPECT_EQ(1u, restarted.ledger_entries(filter).size());
  auto mix = restarted.recommend_mix("KES", 1600);
  EXPECT_EQ(1, mix["1000"]);
  EXPECT_EQ(1, mix["500"]);
  EXPECT_EQ(1, mix["100"]);
}

TEST(CashIntelligence, ReconciliationBalancedVarianceFraudDuplicateAndMissing) {
  CashIntelligenceService svc("");
  EXPECT_TRUE(svc.add_ledger_entry(entry("txn-1", "transaction", 1000, 1000)));
  auto balanced = svc.reconcile("device", "dev-1", "KES", 1000, 40000);
  EXPECT_EQ("balanced", balanced.status);

  EXPECT_TRUE(svc.add_ledger_entry(entry("txn-2", "transaction", 10000, 5000)));
  auto loss = svc.reconcile("device", "dev-1", "KES", 5000, 40100);
  EXPECT_EQ("suspected_loss", loss.status);
  EXPECT_NE(loss.findings.end(), std::find(loss.findings.begin(), loss.findings.end(), "shortage"));

  EXPECT_TRUE(svc.add_ledger_entry(entry("txn-3", "drawer_open", 0, 0, 100)));
  auto fraud = svc.reconcile("device", "dev-1", "KES", 100, 40200);
  EXPECT_EQ("suspected_fraud", fraud.status);

  EXPECT_TRUE(svc.add_ledger_entry(entry("txn-3", "transaction", 100, 100)));
  auto duplicate = svc.reconcile("device", "dev-1", "KES", 100, 40300);
  EXPECT_EQ("unresolved", duplicate.status);

  auto missing = svc.reconcile("branch", "missing-branch", "KES", 0, 40400);
  EXPECT_EQ("unresolved", missing.status);
  EXPECT_NE(missing.findings.end(), std::find(missing.findings.begin(), missing.findings.end(), "missing_events"));
}

TEST(CashIntelligence, ForecastAnomaliesHealthDashboardAndFleetAlerts) {
  cloud::fleet_control::FleetControlPlane fleet("", 30);
  ASSERT_TRUE(fleet.register_device(device("dev-1", "org-1")));
  CashIntelligenceService svc("", &fleet);
  EXPECT_TRUE(svc.add_ledger_entry(entry("txn-1", "transaction", 12000, 10000, 36000)));
  EXPECT_TRUE(svc.add_ledger_entry(entry("txn-2", "transaction", 12000, 9000, 39600)));
  EXPECT_TRUE(svc.add_ledger_entry(entry("txn-3", "transaction", 12000, 7000, 43200)));

  auto forecast = svc.forecast("device", "dev-1", "KES", 50000);
  EXPECT_EQ("medium", forecast.confidence);
  EXPECT_GT(forecast.expected_depletion_epoch, 0);

  EXPECT_TRUE(svc.add_ledger_entry(entry("txn-4", "transaction", 12000, 0, 46800)));
  EXPECT_TRUE(svc.add_ledger_entry(entry("open-1", "drawer_open", 0, 0, 100)));
  auto anomalies = svc.detect_anomalies("device", "dev-1", 50100);
  EXPECT_FALSE(anomalies.empty());
  EXPECT_FALSE(fleet.alerts("org-1").empty());

  auto report = svc.reconcile("device", "dev-1", "KES", 5000, 50200);
  EXPECT_NE("balanced", report.status);
  auto health = svc.health_score("device", "dev-1");
  EXPECT_LT(health.score, 100);
  EXPECT_FALSE(health.explanations.empty());

  auto dash = svc.dashboard("device", "dev-1", "KES");
  EXPECT_EQ("device", dash.scope_type);
  EXPECT_FALSE(dash.variance_trend.empty());
  EXPECT_FALSE(dash.anomaly_summary.empty());
  EXPECT_FALSE(dash.replenishment_recommendations.empty());
}

TEST(CashIntelligenceApi, RequiresAuthAndFiltersByTenant) {
  cloud::fleet_control::FleetControlPlane fleet("", 30);
  fleet.register_device(device("dev-1", "org-1"));
  fleet.register_device(device("dev-2", "org-2"));
  CashIntelligenceService svc("", &fleet);
  ASSERT_TRUE(svc.add_ledger_entry(entry("txn-1", "transaction", 1000, 1000)));
  auto other = entry("txn-2", "transaction", 1000, 1000);
  other.device_id = "dev-2";
  ASSERT_TRUE(svc.add_ledger_entry(other));

  RunningServer srv;
  register_cash_intelligence_routes(srv.server, svc, bearer_auth);
  srv.start();

  httplib::Client cli("127.0.0.1", srv.port);
  auto unauthorized = cli.Get("/cash/v1/ledger");
  ASSERT_TRUE(unauthorized);
  EXPECT_EQ(401, unauthorized->status);

  httplib::Headers org1{{"Authorization", "Bearer token"}, {"X-Org-Id", "org-1"}};
  auto res = cli.Get("/cash/v1/ledger", org1);
  ASSERT_TRUE(res);
  EXPECT_EQ(200, res->status);
  auto body = nlohmann::json::parse(res->body);
  ASSERT_EQ(1u, body["entries"].size());
  EXPECT_EQ("dev-1", body["entries"][0]["device_id"]);
}
