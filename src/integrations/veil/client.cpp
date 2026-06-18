#include "integrations/veil/client.hpp"

namespace integrations::veil {

bool LocalVeilClient::submit_trust_event(const TrustEvidenceRecord& evidence) {
  if (!available_) return false;
  submitted_events_.push_back(evidence);
  return true;
}

bool LocalVeilClient::submit_evidence_bundle(const EvidenceBundle& bundle) {
  if (!available_) return false;
  submitted_bundles_.push_back(bundle);
  return true;
}

PolicyDecision LocalVeilClient::verify_policy(const std::string& tenant_id, const std::string& device_id,
                                              const std::string& action, const nlohmann::json& context) {
  PolicyDecision d;
  d.tenant_id = tenant_id;
  d.device_id = device_id;
  d.action = action;
  d.context = context;
  if (!available_) {
    d.unavailable = true;
    d.allowed = false;
    d.reason = "veil_unavailable";
    return d;
  }
  d.allowed = denied_actions_.count(action) == 0;
  d.reason = d.allowed ? "local_policy_allow" : "local_policy_deny";
  return d;
}

PolicyDecision LocalVeilClient::request_restore_authorization(const std::string& tenant_id,
                                                             const std::string& device_id,
                                                             const nlohmann::json& context) {
  auto d = verify_policy(tenant_id, device_id, "request_restore_authorization", context);
  if (d.allowed) d.reason = "restore_authorization_placeholder";
  return d;
}

std::optional<TrustScoreSummary> LocalVeilClient::fetch_trust_score(const std::string& tenant_id,
                                                                    const std::string& device_id) {
  if (!available_) return std::nullopt;
  TrustScoreSummary summary;
  summary.tenant_id = tenant_id;
  summary.device_id = device_id;
  summary.score = 100;
  summary.factors = {{"source", "local_placeholder"}};
  summary.explanations = {"local VEIL client placeholder score"};
  return summary;
}

}  // namespace integrations::veil
