#include "integrations/veil/service.hpp"

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>

#include <openssl/sha.h>

#include "cloud/cash_intelligence/routes.hpp"
#include "cloud/fleet_control/routes.hpp"
#include "obs/metrics.hpp"

namespace integrations::veil {
namespace {

std::string id_for(const std::string& prefix, size_t n) {
  return prefix + "-" + std::to_string(n + 1);
}

std::string chain_key(const std::string& kind, const std::string& value) {
  return kind + ":" + value;
}

bool env_true(const char* name) {
  const char* value = std::getenv(name);
  return value && (std::string(value) == "1" || std::string(value) == "true" ||
                   std::string(value) == "TRUE" || std::string(value) == "yes");
}

bool production_mode() {
  const char* env = std::getenv("REGISTER_MVP_ENV");
  const char* node_env = std::getenv("NODE_ENV");
  return env_true("REGISTER_MVP_PRODUCTION") ||
         (env && std::string(env) == "production") ||
         (node_env && std::string(node_env) == "production");
}

}  // namespace

std::string sha256_hex(const std::string& input) {
  unsigned char hash[SHA256_DIGEST_LENGTH];
  SHA256(reinterpret_cast<const unsigned char*>(input.data()), input.size(), hash);
  std::ostringstream out;
  for (unsigned char byte : hash) {
    out << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byte);
  }
  return out.str();
}

VeilTrustService::VeilTrustService(std::string store_path, VeilClient* client,
                                   cloud::fleet_control::FleetControlPlane* fleet,
                                   cloud::cash_intelligence::CashIntelligenceService* cash,
                                   PolicyConfig policy)
    : store_path_(std::move(store_path)),
      client_(client ? client : &owned_client_),
      fleet_(fleet),
      cash_(cash),
      policy_(std::move(policy)) {
  load();
}

TrustEvidenceRecord VeilTrustService::create_evidence(TrustEvidenceRecord evidence) {
  std::lock_guard<std::mutex> lk(mu_);
  if (evidence.evidence_id.empty()) evidence.evidence_id = id_for("evidence", evidence_.size());
  if (evidence.audit_event_id.empty()) evidence.audit_event_id = id_for("trust-audit", audit_events_.size());
  if (evidence.classification_labels.empty()) evidence.classification_labels = {"operational"};
  if (evidence.sensitivity_labels.empty()) evidence.sensitivity_labels = {"internal"};
  evidence.payload_hash = canonical_payload_hash(evidence);
  auto device_prev = evidence.device_id.empty() ? "" : last_hash_for_locked(chain_key("device", evidence.device_id));
  auto tenant_prev = evidence.tenant_id.empty() ? "" : last_hash_for_locked(chain_key("tenant", evidence.tenant_id));
  evidence.previous_hash = !device_prev.empty() ? device_prev : tenant_prev;
  evidence.local_signature = "local-test-signature:" + sha256_hex(evidence.payload_hash + evidence.previous_hash);
  evidence_.push_back(evidence);
  audit_locked(evidence, "trust_evidence_created");
  save_locked();
  return evidence;
}

bool VeilTrustService::submit_evidence(const std::string& evidence_id) {
  std::lock_guard<std::mutex> lk(mu_);
  auto it = std::find_if(evidence_.begin(), evidence_.end(), [&](const auto& e) {
    return e.evidence_id == evidence_id;
  });
  if (it == evidence_.end()) return false;
  bool ok = client_ && client_->submit_trust_event(*it);
  audit_locked(*it, ok ? "trust_evidence_submitted" : "trust_evidence_submission_failed");
  save_locked();
  return ok;
}

bool VeilTrustService::submit_bundle(const EvidenceBundle& bundle) {
  std::lock_guard<std::mutex> lk(mu_);
  bool ok = client_ && client_->submit_evidence_bundle(bundle);
  audit_locked(bundle.records.empty() ? "" : bundle.records.front().tenant_id,
               bundle.records.empty() ? "" : bundle.records.front().device_id,
               ok ? "trust_evidence_submitted" : "trust_evidence_submission_failed",
               bundle.end_time, {{"bundle_id", bundle.bundle_id}});
  save_locked();
  return ok;
}

bool VeilTrustService::filter_matches(const TrustEvidenceRecord& evidence,
                                      const EvidenceFilter& filter) const {
  if (!filter.tenant_id.empty() && evidence.tenant_id != filter.tenant_id) return false;
  if (!filter.device_id.empty() && evidence.device_id != filter.device_id) return false;
  if (!filter.merchant_id.empty() && evidence.merchant_id != filter.merchant_id) return false;
  if (!filter.branch_id.empty() && evidence.branch_id != filter.branch_id) return false;
  if (!filter.region.empty() && evidence.region != filter.region) return false;
  if (!filter.event_type.empty() && evidence.event_type != filter.event_type) return false;
  if (filter.start_time > 0 && evidence.timestamp < filter.start_time) return false;
  if (filter.end_time > 0 && evidence.timestamp > filter.end_time) return false;
  return true;
}

std::vector<TrustEvidenceRecord> VeilTrustService::list_evidence(const EvidenceFilter& filter) const {
  std::lock_guard<std::mutex> lk(mu_);
  std::vector<TrustEvidenceRecord> out;
  for (const auto& evidence : evidence_) {
    if (filter_matches(evidence, filter)) out.push_back(evidence);
  }
  std::sort(out.begin(), out.end(), [](const auto& a, const auto& b) {
    if (a.timestamp != b.timestamp) return a.timestamp < b.timestamp;
    return a.evidence_id < b.evidence_id;
  });
  return out;
}

ChainVerificationResult VeilTrustService::verify_chain(const EvidenceFilter& filter) {
  std::lock_guard<std::mutex> lk(mu_);
  auto result = verify_chain_locked(filter);
  audit_locked(filter.tenant_id, filter.device_id,
               result.valid ? "trust_chain_verified" : "trust_chain_verification_failed",
               0, {{"records_checked", result.records_checked}, {"errors", result.errors}});
  save_locked();
  return result;
}

ChainVerificationResult VeilTrustService::verify_chain_locked(const EvidenceFilter& filter) {
  ChainVerificationResult result;
  result.tenant_id = filter.tenant_id;
  result.device_id = filter.device_id;
  std::set<std::string> ids;
  std::unordered_map<std::string, std::string> last_hash;
  std::unordered_map<std::string, long> last_time;
  std::vector<TrustEvidenceRecord> records;
  for (const auto& evidence : evidence_) {
    if (filter_matches(evidence, filter)) records.push_back(evidence);
  }
  for (const auto& evidence : records) {
    ++result.records_checked;
    if (!ids.insert(evidence.evidence_id).second) {
      result.valid = false;
      result.errors.push_back("duplicate_evidence_id:" + evidence.evidence_id);
    }
    auto expected_payload = canonical_payload_hash(evidence);
    if (expected_payload != evidence.payload_hash) {
      result.valid = false;
      result.errors.push_back("payload_tamper:" + evidence.evidence_id);
    }
    std::vector<std::string> keys;
    if (!evidence.device_id.empty()) keys.push_back(chain_key("device", evidence.device_id));
    if (!evidence.tenant_id.empty()) keys.push_back(chain_key("tenant", evidence.tenant_id));
    for (const auto& key : keys) {
      auto prev = last_hash.find(key) == last_hash.end() ? "" : last_hash[key];
      if (!prev.empty() && evidence.previous_hash != prev) {
        result.valid = false;
        result.errors.push_back("missing_link:" + evidence.evidence_id);
      }
      if (last_time.count(key) && evidence.timestamp < last_time[key]) {
        result.valid = false;
        result.errors.push_back("out_of_order:" + evidence.evidence_id);
      }
      last_hash[key] = record_chain_hash(evidence);
      last_time[key] = evidence.timestamp;
    }
  }
  nlohmann::json summary = nlohmann::json::array();
  for (const auto& evidence : records) summary.push_back(record_chain_hash(evidence));
  result.hash_summary = sha256_hex(summary.dump());
  return result;
}

bool VeilTrustService::scope_matches(const TrustEvidenceRecord& evidence, const std::string& scope_type,
                                     const std::string& scope_id) const {
  if (scope_type == "device") return evidence.device_id == scope_id;
  if (scope_type == "merchant" || scope_type == "reconciliation") return evidence.merchant_id == scope_id;
  if (scope_type == "branch") return evidence.branch_id == scope_id;
  if (scope_type == "region") return evidence.region == scope_id;
  if (scope_type == "anomaly") return evidence.correlation_id == scope_id || evidence.event_type == "anomaly_detected";
  if (scope_type == "ota") return evidence.event_type.find("ota_") == 0 && (scope_id.empty() || evidence.device_id == scope_id);
  return false;
}

EvidenceBundle VeilTrustService::export_bundle(const std::string& scope_type, const std::string& scope_id,
                                               long start_time, long end_time) {
  std::lock_guard<std::mutex> lk(mu_);
  EvidenceBundle bundle;
  bundle.bundle_id = "bundle-" + sha256_hex(scope_type + ":" + scope_id + ":" +
                                            std::to_string(start_time) + ":" + std::to_string(end_time)).substr(0, 16);
  bundle.scope_type = scope_type;
  bundle.scope_id = scope_id;
  bundle.start_time = start_time;
  bundle.end_time = end_time;
  for (const auto& evidence : evidence_) {
    if (!scope_matches(evidence, scope_type, scope_id)) continue;
    if (start_time > 0 && evidence.timestamp < start_time) continue;
    if (end_time > 0 && evidence.timestamp > end_time) continue;
    bundle.records.push_back(evidence);
    if (!evidence.audit_event_id.empty()) bundle.related_audit_events.push_back(evidence.audit_event_id);
  }
  std::sort(bundle.records.begin(), bundle.records.end(), [](const auto& a, const auto& b) {
    if (a.timestamp != b.timestamp) return a.timestamp < b.timestamp;
    return a.evidence_id < b.evidence_id;
  });
  EvidenceFilter filter;
  if (scope_type == "device") filter.device_id = scope_id;
  if (!bundle.records.empty()) filter.tenant_id = bundle.records.front().tenant_id;
  filter.start_time = start_time;
  filter.end_time = end_time;
  bundle.verification = verify_chain_locked(filter);
  nlohmann::json hashes = nlohmann::json::array();
  for (const auto& evidence : bundle.records) hashes.push_back(record_chain_hash(evidence));
  bundle.hash_summary = sha256_hex(hashes.dump());
  for (const auto& decision : decisions_) {
    if (!bundle.records.empty() && decision.tenant_id == bundle.records.front().tenant_id) {
      bundle.policy_decisions.push_back(decision);
    }
  }
  audit_locked(bundle.records.empty() ? "" : bundle.records.front().tenant_id,
               bundle.records.empty() ? "" : bundle.records.front().device_id,
               "trust_bundle_exported", end_time, {{"bundle_id", bundle.bundle_id}});
  save_locked();
  return bundle;
}

PolicyDecision VeilTrustService::verify_policy(const std::string& tenant_id, const std::string& device_id,
                                               const std::string& action, const nlohmann::json& context,
                                               long now_epoch) {
  std::lock_guard<std::mutex> lk(mu_);
  PolicyDecision decision;
  if (client_) decision = client_->verify_policy(tenant_id, device_id, action, context);
  decision.decision_id = id_for("veil-policy", decisions_.size());
  decision.tenant_id = tenant_id;
  decision.device_id = device_id;
  decision.action = action;
  decision.context = context;
  decision.decided_at = now_epoch;
  bool high_risk = policy_.high_risk_actions.count(action) > 0;
  if (decision.unavailable && high_risk) {
    if (policy_.production_mode && policy_.fail_closed) {
      decision.allowed = false;
      decision.reason = "policy_unavailable_fail_closed";
    } else if (!policy_.production_mode && policy_.local_allow_when_unavailable) {
      decision.allowed = true;
      decision.reason = "policy_unavailable_local_allow";
    }
  }
  decisions_.push_back(decision);
  audit_locked(tenant_id, device_id,
               decision.unavailable ? "veil_policy_unavailable" :
               decision.allowed ? "veil_policy_allowed" : "veil_policy_denied",
               now_epoch, {{"action", action}, {"reason", decision.reason}});
  save_locked();
  return decision;
}

bool VeilTrustService::policy_allows_or_fail_closed(const std::string& tenant_id, const std::string& device_id,
                                                    const std::string& action, const nlohmann::json& context,
                                                    long now_epoch) {
  return verify_policy(tenant_id, device_id, action, context, now_epoch).allowed;
}

TrustScoreSummary VeilTrustService::trust_score(const std::string& tenant_id, const std::string& device_id,
                                                long now_epoch) {
  std::lock_guard<std::mutex> lk(mu_);
  TrustScoreSummary summary;
  summary.tenant_id = tenant_id;
  summary.device_id = device_id;
  summary.updated_at = now_epoch;
  int score = 100;
  if (fleet_) {
    auto device = fleet_->device(device_id);
    if (device) {
      summary.merchant_id = device->merchant_id;
      summary.branch_id = device->branch_id;
      summary.region = device->region;
      int fleet_penalty = std::max(0, 100 - device->health_score);
      if (device->connectivity_status == "offline") fleet_penalty += 15;
      if (device->certificate_identity_status != "active") fleet_penalty += 20;
      if (device->ota_failures > 0) fleet_penalty += device->ota_failures * 10;
      summary.factors["fleet_health"] = std::to_string(std::max(0, 100 - fleet_penalty));
      if (fleet_penalty > 0) summary.explanations.push_back("fleet health, certificate, OTA, or offline state reduced score");
      score -= fleet_penalty;
    }
  }
  if (cash_) {
    auto cash_score = cash_->health_score("device", device_id);
    int cash_penalty = std::max(0, 100 - cash_score.score);
    summary.factors["cash_health"] = std::to_string(cash_score.score);
    if (cash_penalty > 0) summary.explanations.push_back("cash health reduced trust score");
    score -= cash_penalty;
  }
  int anomaly_penalty = 0;
  int variance_penalty = 0;
  for (const auto& evidence : evidence_) {
    if (evidence.tenant_id != tenant_id || evidence.device_id != device_id) continue;
    if (evidence.event_type == "anomaly_detected") anomaly_penalty += 10;
    if (evidence.event_type == "reconciliation_variance_detected") variance_penalty += 15;
  }
  summary.factors["anomaly_severity"] = std::to_string(std::max(0, 100 - anomaly_penalty));
  summary.factors["reconciliation_variance"] = std::to_string(std::max(0, 100 - variance_penalty));
  if (anomaly_penalty > 0) summary.explanations.push_back("cash anomalies reduced trust score");
  if (variance_penalty > 0) summary.explanations.push_back("reconciliation variance reduced trust score");
  score -= anomaly_penalty + variance_penalty;
  summary.score = std::max(0, score);
  audit_locked(tenant_id, device_id, "trust_score_updated", now_epoch, {{"score", summary.score}});
  obs::M().gauge("register_veil_trust_score", "VEIL trust score",
                 {{"tenant_id", tenant_id}, {"device_id", device_id}})
      .set(summary.score);
  save_locked();
  return summary;
}

std::vector<PolicyDecision> VeilTrustService::policy_decisions(const std::string& tenant_id) const {
  std::lock_guard<std::mutex> lk(mu_);
  std::vector<PolicyDecision> out;
  for (const auto& decision : decisions_) {
    if (tenant_id.empty() || decision.tenant_id == tenant_id) out.push_back(decision);
  }
  return out;
}

std::string VeilTrustService::canonical_payload_hash(const TrustEvidenceRecord& evidence) const {
  nlohmann::json payload = {{"tenant_id", evidence.tenant_id},
                            {"device_id", evidence.device_id},
                            {"merchant_id", evidence.merchant_id},
                            {"branch_id", evidence.branch_id},
                            {"region", evidence.region},
                            {"event_type", evidence.event_type},
                            {"source_module", evidence.source_module},
                            {"timestamp", evidence.timestamp},
                            {"policy_context", evidence.policy_context},
                            {"payload", evidence.payload},
                            {"audit_event_id", evidence.audit_event_id},
                            {"correlation_id", evidence.correlation_id}};
  return sha256_hex(payload.dump());
}

std::string VeilTrustService::record_chain_hash(const TrustEvidenceRecord& evidence) const {
  return sha256_hex(evidence.evidence_id + evidence.payload_hash + evidence.previous_hash +
                    evidence.local_signature);
}

std::string VeilTrustService::last_hash_for_locked(const std::string& key) const {
  for (auto it = evidence_.rbegin(); it != evidence_.rend(); ++it) {
    if (key == chain_key("device", it->device_id) || key == chain_key("tenant", it->tenant_id)) {
      return record_chain_hash(*it);
    }
  }
  return "";
}

void VeilTrustService::audit_locked(const TrustEvidenceRecord& evidence, const std::string& event_type,
                                    nlohmann::json metadata) {
  audit_locked(evidence.tenant_id, evidence.device_id, event_type, evidence.timestamp, std::move(metadata));
}

void VeilTrustService::audit_locked(const std::string& tenant_id, const std::string& device_id,
                                    const std::string& event_type, long timestamp,
                                    nlohmann::json metadata) {
  audit_events_.push_back({{"event_id", id_for("trust-event", audit_events_.size())},
                           {"tenant_id", tenant_id},
                           {"device_id", device_id},
                           {"event_type", event_type},
                           {"timestamp", timestamp},
                           {"metadata", metadata}});
}

void VeilTrustService::save() const {
  std::lock_guard<std::mutex> lk(mu_);
  save_locked();
}

void VeilTrustService::load() {
  if (store_path_.empty()) return;
  std::ifstream in(store_path_);
  if (!in) return;
  nlohmann::json j;
  in >> j;
  evidence_ = j.value("evidence", evidence_);
  decisions_ = j.value("policy_decisions", decisions_);
  audit_events_ = j.value("audit_events", audit_events_);
}

void VeilTrustService::save_locked() const {
  if (store_path_.empty()) return;
  auto parent = std::filesystem::path(store_path_).parent_path();
  if (!parent.empty()) std::filesystem::create_directories(parent);
  std::ofstream(store_path_, std::ios::trunc)
      << nlohmann::json{{"schema", 1},
                        {"evidence", evidence_},
                        {"policy_decisions", decisions_},
                        {"audit_events", audit_events_}}
             .dump(2)
      << "\n";
}

VeilTrustService& default_trust_service() {
  static VeilTrustService* svc = [] {
    const char* env = std::getenv("REGISTER_MVP_VEIL_STORE");
    std::string path = env && *env ? std::string(env) : "data/veil_trust_evidence.json";
    PolicyConfig policy;
    policy.production_mode = production_mode();
    policy.fail_closed = true;
    policy.local_allow_when_unavailable = !policy.production_mode;
    return new VeilTrustService(path, nullptr, &cloud::fleet_control::default_control_plane(),
                                &cloud::cash_intelligence::default_cash_service(), policy);
  }();
  return *svc;
}

}  // namespace integrations::veil
