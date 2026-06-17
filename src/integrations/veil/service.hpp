#pragma once

#include <mutex>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

#include "cloud/cash_intelligence/service.hpp"
#include "cloud/fleet_control/control_plane.hpp"
#include "integrations/veil/client.hpp"

namespace integrations::veil {

struct PolicyConfig {
  bool production_mode{false};
  bool fail_closed{true};
  bool local_allow_when_unavailable{true};
  std::set<std::string> high_risk_actions{
      "disable_device", "enable_device", "pause_transactions", "resume_transactions",
      "ota_promotion", "ota_install_eligibility", "certificate_revocation",
      "reconciliation_override", "manual_adjustment", "command_execution"};
};

class VeilTrustService {
 public:
  explicit VeilTrustService(std::string store_path = "", VeilClient* client = nullptr,
                            cloud::fleet_control::FleetControlPlane* fleet = nullptr,
                            cloud::cash_intelligence::CashIntelligenceService* cash = nullptr,
                            PolicyConfig policy = {});

  TrustEvidenceRecord create_evidence(TrustEvidenceRecord evidence);
  bool submit_evidence(const std::string& evidence_id);
  bool submit_bundle(const EvidenceBundle& bundle);
  std::vector<TrustEvidenceRecord> list_evidence(const EvidenceFilter& filter = {}) const;
  ChainVerificationResult verify_chain(const EvidenceFilter& filter = {});
  EvidenceBundle export_bundle(const std::string& scope_type, const std::string& scope_id,
                               long start_time = 0, long end_time = 0);

  PolicyDecision verify_policy(const std::string& tenant_id, const std::string& device_id,
                               const std::string& action, const nlohmann::json& context,
                               long now_epoch = 0);
  bool policy_allows_or_fail_closed(const std::string& tenant_id, const std::string& device_id,
                                    const std::string& action, const nlohmann::json& context,
                                    long now_epoch = 0);
  TrustScoreSummary trust_score(const std::string& tenant_id, const std::string& device_id,
                                long now_epoch = 0);
  std::vector<PolicyDecision> policy_decisions(const std::string& tenant_id = "") const;

  void save() const;
  void load();

 private:
  bool filter_matches(const TrustEvidenceRecord& evidence, const EvidenceFilter& filter) const;
  bool scope_matches(const TrustEvidenceRecord& evidence, const std::string& scope_type,
                     const std::string& scope_id) const;
  std::string canonical_payload_hash(const TrustEvidenceRecord& evidence) const;
  std::string record_chain_hash(const TrustEvidenceRecord& evidence) const;
  ChainVerificationResult verify_chain_locked(const EvidenceFilter& filter);
  std::string last_hash_for_locked(const std::string& key) const;
  void audit_locked(const TrustEvidenceRecord& evidence, const std::string& event_type,
                    nlohmann::json metadata = {});
  void audit_locked(const std::string& tenant_id, const std::string& device_id,
                    const std::string& event_type, long timestamp, nlohmann::json metadata = {});
  void save_locked() const;

  mutable std::mutex mu_;
  std::string store_path_;
  VeilClient* client_{nullptr};
  LocalVeilClient owned_client_;
  cloud::fleet_control::FleetControlPlane* fleet_{nullptr};
  cloud::cash_intelligence::CashIntelligenceService* cash_{nullptr};
  PolicyConfig policy_;
  std::vector<TrustEvidenceRecord> evidence_;
  std::vector<PolicyDecision> decisions_;
  std::vector<nlohmann::json> audit_events_;
};

std::string sha256_hex(const std::string& input);
VeilTrustService& default_trust_service();

}  // namespace integrations::veil
