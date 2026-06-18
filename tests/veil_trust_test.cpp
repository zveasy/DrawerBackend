#include <gtest/gtest.h>

#include <filesystem>
#include <functional>
#include <fstream>
#include <thread>

#include <httplib.h>
#include <nlohmann/json.hpp>

#include "cloud/fleet_control/control_plane.hpp"
#include "integrations/veil/routes.hpp"

using namespace integrations::veil;

namespace {

TrustEvidenceRecord evidence(std::string id, std::string event_type, long ts = 100) {
  TrustEvidenceRecord e;
  e.evidence_id = std::move(id);
  e.tenant_id = "org-1";
  e.device_id = "dev-1";
  e.merchant_id = "merchant-1";
  e.branch_id = "branch-1";
  e.region = "KE-NBO";
  e.event_type = std::move(event_type);
  e.source_module = "test";
  e.timestamp = ts;
  e.payload = {{"amount", 1000}, {"currency", "KES"}};
  e.policy_context = {{"risk", "pilot"}};
  e.classification_labels = {"financial_edge"};
  e.sensitivity_labels = {"cash_ops"};
  e.audit_event_id = "audit-" + e.evidence_id;
  e.correlation_id = "corr-1";
  return e;
}

cloud::fleet_control::DeviceRegistryRecord device() {
  cloud::fleet_control::DeviceRegistryRecord d;
  d.device_id = "dev-1";
  d.organization_id = "org-1";
  d.merchant_id = "merchant-1";
  d.branch_id = "branch-1";
  d.region = "KE-NBO";
  d.connectivity_status = "offline";
  d.certificate_identity_status = "expired";
  d.health_score = 70;
  d.ota_failures = 1;
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

void rewrite_store(const std::filesystem::path& path, const std::function<void(nlohmann::json&)>& fn) {
  std::ifstream in(path);
  nlohmann::json j;
  in >> j;
  fn(j);
  std::ofstream(path, std::ios::trunc) << j.dump(2) << "\n";
}

}  // namespace

TEST(VeilTrust, LocalClientSubmitsAndDeniesPolicy) {
  LocalVeilClient client;
  auto decision = client.verify_policy("org-1", "dev-1", "disable_device", {});
  EXPECT_TRUE(decision.allowed);
  client.deny_action("disable_device");
  auto denied = client.verify_policy("org-1", "dev-1", "disable_device", {});
  EXPECT_FALSE(denied.allowed);

  EXPECT_TRUE(client.submit_trust_event(evidence("e-1", "device_heartbeat")));
  EXPECT_EQ(1u, client.submitted_events().size());
  auto restore = client.request_restore_authorization("org-1", "dev-1", {});
  EXPECT_TRUE(restore.allowed);
  EXPECT_TRUE(client.fetch_trust_score("org-1", "dev-1").has_value());
}

TEST(VeilTrust, EvidenceCreationHashChainingAndBundleExportAreDeterministic) {
  LocalVeilClient client;
  VeilTrustService svc("", &client);
  auto first = svc.create_evidence(evidence("e-1", "device_enrollment", 100));
  auto second = svc.create_evidence(evidence("e-2", "command_created", 120));

  EXPECT_FALSE(first.payload_hash.empty());
  EXPECT_EQ(first.local_signature.substr(0, 21), "local-test-signature:");
  EXPECT_FALSE(second.previous_hash.empty());
  EvidenceFilter filter;
  filter.tenant_id = "org-1";
  filter.device_id = "dev-1";
  auto verification = svc.verify_chain(filter);
  EXPECT_TRUE(verification.valid);
  EXPECT_EQ(2, verification.records_checked);

  auto bundle1 = svc.export_bundle("device", "dev-1", 0, 200);
  auto bundle2 = svc.export_bundle("device", "dev-1", 0, 200);
  EXPECT_EQ(bundle1.hash_summary, bundle2.hash_summary);
  EXPECT_EQ(2u, bundle1.records.size());
  EXPECT_TRUE(svc.submit_bundle(bundle1));
  EXPECT_EQ(1u, client.submitted_bundles().size());
}

TEST(VeilTrust, VerificationDetectsTamperMissingDuplicateAndOutOfOrder) {
  auto path = std::filesystem::temp_directory_path() / "veil_trust_store.json";
  std::filesystem::remove(path);
  {
    VeilTrustService svc(path.string());
    svc.create_evidence(evidence("e-1", "cash_ledger_entry", 100));
    svc.create_evidence(evidence("e-2", "reconciliation_variance_detected", 120));
    svc.create_evidence(evidence("e-3", "anomaly_detected", 90));
  }

  rewrite_store(path, [](nlohmann::json& j) {
    j["evidence"][0]["payload"]["amount"] = 2000;
    j["evidence"][1]["previous_hash"] = "broken";
    j["evidence"].push_back(j["evidence"][1]);
  });

  VeilTrustService tampered(path.string());
  EvidenceFilter filter;
  filter.tenant_id = "org-1";
  filter.device_id = "dev-1";
  auto verification = tampered.verify_chain(filter);
  EXPECT_FALSE(verification.valid);
  auto errors = nlohmann::json(verification.errors).dump();
  EXPECT_NE(std::string::npos, errors.find("payload_tamper"));
  EXPECT_NE(std::string::npos, errors.find("missing_link"));
  EXPECT_NE(std::string::npos, errors.find("duplicate_evidence_id"));
  EXPECT_NE(std::string::npos, errors.find("out_of_order"));
}

TEST(VeilTrust, PolicyUnavailableFailClosedInProductionAndLocalAllowInDev) {
  LocalVeilClient client;
  client.set_available(false);

  PolicyConfig prod;
  prod.production_mode = true;
  prod.fail_closed = true;
  prod.local_allow_when_unavailable = false;
  VeilTrustService prod_svc("", &client, nullptr, nullptr, prod);
  auto denied = prod_svc.verify_policy("org-1", "dev-1", "disable_device", {}, 100);
  EXPECT_FALSE(denied.allowed);
  EXPECT_EQ("policy_unavailable_fail_closed", denied.reason);
  EXPECT_FALSE(prod_svc.policy_allows_or_fail_closed("org-1", "dev-1", "command_execution", {}, 101));

  PolicyConfig dev;
  dev.production_mode = false;
  dev.local_allow_when_unavailable = true;
  VeilTrustService dev_svc("", &client, nullptr, nullptr, dev);
  auto allowed = dev_svc.verify_policy("org-1", "dev-1", "disable_device", {}, 100);
  EXPECT_TRUE(allowed.allowed);
  EXPECT_EQ("policy_unavailable_local_allow", allowed.reason);
}

TEST(VeilTrust, TrustScoreExplainabilityUsesFleetAndEvidenceSignals) {
  cloud::fleet_control::FleetControlPlane fleet("", 30);
  ASSERT_TRUE(fleet.register_device(device()));
  VeilTrustService svc("", nullptr, &fleet);
  svc.create_evidence(evidence("e-1", "anomaly_detected", 100));
  svc.create_evidence(evidence("e-2", "reconciliation_variance_detected", 120));

  auto score = svc.trust_score("org-1", "dev-1", 130);
  EXPECT_LT(score.score, 100);
  EXPECT_FALSE(score.explanations.empty());
  EXPECT_EQ("merchant-1", score.merchant_id);
}

TEST(VeilTrustApi, RequiresAuthAndIsolatesTenants) {
  VeilTrustService svc("");
  svc.create_evidence(evidence("e-1", "device_heartbeat", 100));
  auto other = evidence("e-2", "device_heartbeat", 110);
  other.tenant_id = "org-2";
  other.device_id = "dev-2";
  svc.create_evidence(other);

  RunningServer srv;
  register_trust_routes(srv.server, svc, bearer_auth);
  srv.start();

  httplib::Client cli("127.0.0.1", srv.port);
  auto unauthorized = cli.Get("/trust/v1/evidence");
  ASSERT_TRUE(unauthorized);
  EXPECT_EQ(401, unauthorized->status);

  httplib::Headers org1{{"Authorization", "Bearer token"}, {"X-Org-Id", "org-1"}};
  auto res = cli.Get("/trust/v1/evidence", org1);
  ASSERT_TRUE(res);
  EXPECT_EQ(200, res->status);
  auto body = nlohmann::json::parse(res->body);
  ASSERT_EQ(1u, body["evidence"].size());
  EXPECT_EQ("org-1", body["evidence"][0]["tenant_id"]);

  auto forbidden = cli.Post("/trust/v1/evidence", org1, nlohmann::json(other).dump(), "application/json");
  ASSERT_TRUE(forbidden);
  EXPECT_EQ(403, forbidden->status);
}
