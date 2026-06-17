#pragma once

#include <optional>
#include <set>
#include <string>
#include <vector>

#include "integrations/veil/models.hpp"

namespace integrations::veil {

class VeilClient {
 public:
  virtual ~VeilClient() = default;
  virtual bool submit_trust_event(const TrustEvidenceRecord& evidence) = 0;
  virtual bool submit_evidence_bundle(const EvidenceBundle& bundle) = 0;
  virtual PolicyDecision verify_policy(const std::string& tenant_id, const std::string& device_id,
                                       const std::string& action, const nlohmann::json& context) = 0;
  virtual PolicyDecision request_restore_authorization(const std::string& tenant_id,
                                                       const std::string& device_id,
                                                       const nlohmann::json& context) = 0;
  virtual std::optional<TrustScoreSummary> fetch_trust_score(const std::string& tenant_id,
                                                             const std::string& device_id) = 0;
};

class LocalVeilClient final : public VeilClient {
 public:
  void set_available(bool available) { available_ = available; }
  void deny_action(std::string action) { denied_actions_.insert(std::move(action)); }
  void clear_denials() { denied_actions_.clear(); }

  bool submit_trust_event(const TrustEvidenceRecord& evidence) override;
  bool submit_evidence_bundle(const EvidenceBundle& bundle) override;
  PolicyDecision verify_policy(const std::string& tenant_id, const std::string& device_id,
                               const std::string& action, const nlohmann::json& context) override;
  PolicyDecision request_restore_authorization(const std::string& tenant_id,
                                               const std::string& device_id,
                                               const nlohmann::json& context) override;
  std::optional<TrustScoreSummary> fetch_trust_score(const std::string& tenant_id,
                                                     const std::string& device_id) override;

  const std::vector<TrustEvidenceRecord>& submitted_events() const { return submitted_events_; }
  const std::vector<EvidenceBundle>& submitted_bundles() const { return submitted_bundles_; }

 private:
  bool available_{true};
  std::set<std::string> denied_actions_;
  std::vector<TrustEvidenceRecord> submitted_events_;
  std::vector<EvidenceBundle> submitted_bundles_;
};

}  // namespace integrations::veil
