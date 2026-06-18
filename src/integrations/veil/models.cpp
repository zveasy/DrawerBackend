#include "integrations/veil/models.hpp"

namespace integrations::veil {

void to_json(nlohmann::json& j, const TrustEvidenceRecord& v) {
  j = {{"evidence_id", v.evidence_id},
       {"tenant_id", v.tenant_id},
       {"device_id", v.device_id},
       {"merchant_id", v.merchant_id},
       {"branch_id", v.branch_id},
       {"region", v.region},
       {"event_type", v.event_type},
       {"source_module", v.source_module},
       {"timestamp", v.timestamp},
       {"payload_hash", v.payload_hash},
       {"previous_hash", v.previous_hash},
       {"local_signature", v.local_signature},
       {"policy_context", v.policy_context},
       {"classification_labels", v.classification_labels},
       {"sensitivity_labels", v.sensitivity_labels},
       {"audit_event_id", v.audit_event_id},
       {"correlation_id", v.correlation_id},
       {"payload", v.payload}};
}

void from_json(const nlohmann::json& j, TrustEvidenceRecord& v) {
  v.evidence_id = j.value("evidence_id", v.evidence_id);
  v.tenant_id = j.value("tenant_id", v.tenant_id);
  v.device_id = j.value("device_id", v.device_id);
  v.merchant_id = j.value("merchant_id", v.merchant_id);
  v.branch_id = j.value("branch_id", v.branch_id);
  v.region = j.value("region", v.region);
  v.event_type = j.value("event_type", v.event_type);
  v.source_module = j.value("source_module", v.source_module);
  v.timestamp = j.value("timestamp", v.timestamp);
  v.payload_hash = j.value("payload_hash", v.payload_hash);
  v.previous_hash = j.value("previous_hash", v.previous_hash);
  v.local_signature = j.value("local_signature", v.local_signature);
  v.policy_context = j.value("policy_context", nlohmann::json::object());
  v.classification_labels = j.value("classification_labels", v.classification_labels);
  v.sensitivity_labels = j.value("sensitivity_labels", v.sensitivity_labels);
  v.audit_event_id = j.value("audit_event_id", v.audit_event_id);
  v.correlation_id = j.value("correlation_id", v.correlation_id);
  v.payload = j.value("payload", nlohmann::json::object());
}

void to_json(nlohmann::json& j, const PolicyDecision& v) {
  j = {{"decision_id", v.decision_id}, {"tenant_id", v.tenant_id}, {"device_id", v.device_id},
       {"action", v.action}, {"allowed", v.allowed}, {"unavailable", v.unavailable},
       {"reason", v.reason}, {"context", v.context}, {"decided_at", v.decided_at}};
}

void from_json(const nlohmann::json& j, PolicyDecision& v) {
  v.decision_id = j.value("decision_id", v.decision_id);
  v.tenant_id = j.value("tenant_id", v.tenant_id);
  v.device_id = j.value("device_id", v.device_id);
  v.action = j.value("action", v.action);
  v.allowed = j.value("allowed", v.allowed);
  v.unavailable = j.value("unavailable", v.unavailable);
  v.reason = j.value("reason", v.reason);
  v.context = j.value("context", nlohmann::json::object());
  v.decided_at = j.value("decided_at", v.decided_at);
}

void to_json(nlohmann::json& j, const ChainVerificationResult& v) {
  j = {{"valid", v.valid}, {"errors", v.errors}, {"records_checked", v.records_checked},
       {"tenant_id", v.tenant_id}, {"device_id", v.device_id}, {"hash_summary", v.hash_summary}};
}

void from_json(const nlohmann::json& j, ChainVerificationResult& v) {
  v.valid = j.value("valid", v.valid);
  v.errors = j.value("errors", v.errors);
  v.records_checked = j.value("records_checked", v.records_checked);
  v.tenant_id = j.value("tenant_id", v.tenant_id);
  v.device_id = j.value("device_id", v.device_id);
  v.hash_summary = j.value("hash_summary", v.hash_summary);
}

void to_json(nlohmann::json& j, const EvidenceBundle& v) {
  j = {{"bundle_id", v.bundle_id}, {"scope_type", v.scope_type}, {"scope_id", v.scope_id},
       {"start_time", v.start_time}, {"end_time", v.end_time}, {"records", v.records},
       {"verification", v.verification}, {"hash_summary", v.hash_summary},
       {"policy_decisions", v.policy_decisions}, {"related_audit_events", v.related_audit_events}};
}

void from_json(const nlohmann::json& j, EvidenceBundle& v) {
  v.bundle_id = j.value("bundle_id", v.bundle_id);
  v.scope_type = j.value("scope_type", v.scope_type);
  v.scope_id = j.value("scope_id", v.scope_id);
  v.start_time = j.value("start_time", v.start_time);
  v.end_time = j.value("end_time", v.end_time);
  v.records = j.value("records", v.records);
  v.verification = j.value("verification", v.verification);
  v.hash_summary = j.value("hash_summary", v.hash_summary);
  v.policy_decisions = j.value("policy_decisions", v.policy_decisions);
  v.related_audit_events = j.value("related_audit_events", v.related_audit_events);
}

void to_json(nlohmann::json& j, const TrustScoreSummary& v) {
  j = {{"tenant_id", v.tenant_id}, {"device_id", v.device_id}, {"merchant_id", v.merchant_id},
       {"branch_id", v.branch_id}, {"region", v.region}, {"score", v.score},
       {"factors", v.factors}, {"explanations", v.explanations}, {"updated_at", v.updated_at}};
}

void from_json(const nlohmann::json& j, TrustScoreSummary& v) {
  v.tenant_id = j.value("tenant_id", v.tenant_id);
  v.device_id = j.value("device_id", v.device_id);
  v.merchant_id = j.value("merchant_id", v.merchant_id);
  v.branch_id = j.value("branch_id", v.branch_id);
  v.region = j.value("region", v.region);
  v.score = j.value("score", v.score);
  v.factors = j.value("factors", v.factors);
  v.explanations = j.value("explanations", v.explanations);
  v.updated_at = j.value("updated_at", v.updated_at);
}

}  // namespace integrations::veil
