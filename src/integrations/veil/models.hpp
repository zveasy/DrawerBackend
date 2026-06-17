#pragma once

#include <map>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

namespace integrations::veil {

using Labels = std::map<std::string, std::string>;

struct TrustEvidenceRecord {
  std::string evidence_id;
  std::string tenant_id;
  std::string device_id;
  std::string merchant_id;
  std::string branch_id;
  std::string region;
  std::string event_type;
  std::string source_module;
  long timestamp{0};
  std::string payload_hash;
  std::string previous_hash;
  std::string local_signature;
  nlohmann::json policy_context = nlohmann::json::object();
  std::vector<std::string> classification_labels;
  std::vector<std::string> sensitivity_labels;
  std::string audit_event_id;
  std::string correlation_id;
  nlohmann::json payload = nlohmann::json::object();
};

struct PolicyDecision {
  std::string decision_id;
  std::string tenant_id;
  std::string device_id;
  std::string action;
  bool allowed{false};
  bool unavailable{false};
  std::string reason;
  nlohmann::json context = nlohmann::json::object();
  long decided_at{0};
};

struct ChainVerificationResult {
  bool valid{true};
  std::vector<std::string> errors;
  int records_checked{0};
  std::string tenant_id;
  std::string device_id;
  std::string hash_summary;
};

struct EvidenceBundle {
  std::string bundle_id;
  std::string scope_type;
  std::string scope_id;
  long start_time{0};
  long end_time{0};
  std::vector<TrustEvidenceRecord> records;
  ChainVerificationResult verification;
  std::string hash_summary;
  std::vector<PolicyDecision> policy_decisions;
  std::vector<std::string> related_audit_events;
};

struct TrustScoreSummary {
  std::string tenant_id;
  std::string device_id;
  std::string merchant_id;
  std::string branch_id;
  std::string region;
  int score{100};
  Labels factors;
  std::vector<std::string> explanations;
  long updated_at{0};
};

struct TrustEvent {
  TrustEvidenceRecord evidence;
};

struct EvidenceFilter {
  std::string tenant_id;
  std::string device_id;
  std::string merchant_id;
  std::string branch_id;
  std::string region;
  std::string event_type;
  long start_time{0};
  long end_time{0};
};

void to_json(nlohmann::json& j, const TrustEvidenceRecord& v);
void from_json(const nlohmann::json& j, TrustEvidenceRecord& v);
void to_json(nlohmann::json& j, const PolicyDecision& v);
void from_json(const nlohmann::json& j, PolicyDecision& v);
void to_json(nlohmann::json& j, const ChainVerificationResult& v);
void from_json(const nlohmann::json& j, ChainVerificationResult& v);
void to_json(nlohmann::json& j, const EvidenceBundle& v);
void from_json(const nlohmann::json& j, EvidenceBundle& v);
void to_json(nlohmann::json& j, const TrustScoreSummary& v);
void from_json(const nlohmann::json& j, TrustScoreSummary& v);

}  // namespace integrations::veil
