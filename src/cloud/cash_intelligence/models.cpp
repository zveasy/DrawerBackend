#include "cloud/cash_intelligence/models.hpp"

namespace cloud::cash_intelligence {

void to_json(nlohmann::json& j, const CashLedgerEntry& v) {
  j = {{"ledger_entry_id", v.ledger_entry_id},
       {"event_type", v.event_type},
       {"event_id", v.event_id},
       {"device_id", v.device_id},
       {"merchant_id", v.merchant_id},
       {"branch_id", v.branch_id},
       {"region", v.region},
       {"currency", v.currency},
       {"denominations", v.denominations},
       {"expected_balance", v.expected_balance},
       {"observed_balance", v.observed_balance},
       {"variance", v.variance},
       {"actor_id", v.actor_id},
       {"actor_role", v.actor_role},
       {"occurred_at_epoch", v.occurred_at_epoch},
       {"audit_event_id", v.audit_event_id},
       {"duplicate", v.duplicate}};
}

void from_json(const nlohmann::json& j, CashLedgerEntry& v) {
  v.ledger_entry_id = j.value("ledger_entry_id", v.ledger_entry_id);
  v.event_type = j.value("event_type", v.event_type);
  v.event_id = j.value("event_id", v.event_id);
  v.device_id = j.value("device_id", v.device_id);
  v.merchant_id = j.value("merchant_id", v.merchant_id);
  v.branch_id = j.value("branch_id", v.branch_id);
  v.region = j.value("region", v.region);
  v.currency = j.value("currency", v.currency);
  v.denominations = j.value("denominations", v.denominations);
  v.expected_balance = j.value("expected_balance", v.expected_balance);
  v.observed_balance = j.value("observed_balance", v.observed_balance);
  v.variance = j.value("variance", v.variance);
  v.actor_id = j.value("actor_id", v.actor_id);
  v.actor_role = j.value("actor_role", v.actor_role);
  v.occurred_at_epoch = j.value("occurred_at_epoch", v.occurred_at_epoch);
  v.audit_event_id = j.value("audit_event_id", v.audit_event_id);
  v.duplicate = j.value("duplicate", v.duplicate);
}

void to_json(nlohmann::json& j, const DenominationInventory& v) {
  j = {{"scope_type", v.scope_type}, {"scope_id", v.scope_id}, {"currency", v.currency},
       {"quantities", v.quantities}, {"updated_at_epoch", v.updated_at_epoch}};
}

void from_json(const nlohmann::json& j, DenominationInventory& v) {
  v.scope_type = j.value("scope_type", v.scope_type);
  v.scope_id = j.value("scope_id", v.scope_id);
  v.currency = j.value("currency", v.currency);
  v.quantities = j.value("quantities", v.quantities);
  v.updated_at_epoch = j.value("updated_at_epoch", v.updated_at_epoch);
}

void to_json(nlohmann::json& j, const ReconciliationReport& v) {
  j = {{"report_id", v.report_id},
       {"scope_type", v.scope_type},
       {"scope_id", v.scope_id},
       {"merchant_id", v.merchant_id},
       {"branch_id", v.branch_id},
       {"region", v.region},
       {"currency", v.currency},
       {"expected_balance", v.expected_balance},
       {"observed_balance", v.observed_balance},
       {"variance", v.variance},
       {"status", v.status},
       {"findings", v.findings},
       {"started_at_epoch", v.started_at_epoch},
       {"completed_at_epoch", v.completed_at_epoch},
       {"audit_event_id", v.audit_event_id}};
}

void from_json(const nlohmann::json& j, ReconciliationReport& v) {
  v.report_id = j.value("report_id", v.report_id);
  v.scope_type = j.value("scope_type", v.scope_type);
  v.scope_id = j.value("scope_id", v.scope_id);
  v.merchant_id = j.value("merchant_id", v.merchant_id);
  v.branch_id = j.value("branch_id", v.branch_id);
  v.region = j.value("region", v.region);
  v.currency = j.value("currency", v.currency);
  v.expected_balance = j.value("expected_balance", v.expected_balance);
  v.observed_balance = j.value("observed_balance", v.observed_balance);
  v.variance = j.value("variance", v.variance);
  v.status = j.value("status", v.status);
  v.findings = j.value("findings", v.findings);
  v.started_at_epoch = j.value("started_at_epoch", v.started_at_epoch);
  v.completed_at_epoch = j.value("completed_at_epoch", v.completed_at_epoch);
  v.audit_event_id = j.value("audit_event_id", v.audit_event_id);
}

void to_json(nlohmann::json& j, const CashForecast& v) {
  j = {{"forecast_id", v.forecast_id},
       {"scope_type", v.scope_type},
       {"scope_id", v.scope_id},
       {"currency", v.currency},
       {"expected_depletion_epoch", v.expected_depletion_epoch},
       {"expected_surplus_epoch", v.expected_surplus_epoch},
       {"recommended_replenishment_epoch", v.recommended_replenishment_epoch},
       {"high_risk_shortage_windows", v.high_risk_shortage_windows},
       {"confidence", v.confidence},
       {"explanation", v.explanation}};
}

void from_json(const nlohmann::json& j, CashForecast& v) {
  v.forecast_id = j.value("forecast_id", v.forecast_id);
  v.scope_type = j.value("scope_type", v.scope_type);
  v.scope_id = j.value("scope_id", v.scope_id);
  v.currency = j.value("currency", v.currency);
  v.expected_depletion_epoch = j.value("expected_depletion_epoch", v.expected_depletion_epoch);
  v.expected_surplus_epoch = j.value("expected_surplus_epoch", v.expected_surplus_epoch);
  v.recommended_replenishment_epoch = j.value("recommended_replenishment_epoch", v.recommended_replenishment_epoch);
  v.high_risk_shortage_windows = j.value("high_risk_shortage_windows", v.high_risk_shortage_windows);
  v.confidence = j.value("confidence", v.confidence);
  v.explanation = j.value("explanation", v.explanation);
}

void to_json(nlohmann::json& j, const CashAnomaly& v) {
  j = {{"anomaly_id", v.anomaly_id},
       {"scope_type", v.scope_type},
       {"scope_id", v.scope_id},
       {"device_id", v.device_id},
       {"merchant_id", v.merchant_id},
       {"branch_id", v.branch_id},
       {"region", v.region},
       {"type", v.type},
       {"severity", v.severity},
       {"explanation", v.explanation},
       {"detected_at_epoch", v.detected_at_epoch}};
}

void from_json(const nlohmann::json& j, CashAnomaly& v) {
  v.anomaly_id = j.value("anomaly_id", v.anomaly_id);
  v.scope_type = j.value("scope_type", v.scope_type);
  v.scope_id = j.value("scope_id", v.scope_id);
  v.device_id = j.value("device_id", v.device_id);
  v.merchant_id = j.value("merchant_id", v.merchant_id);
  v.branch_id = j.value("branch_id", v.branch_id);
  v.region = j.value("region", v.region);
  v.type = j.value("type", v.type);
  v.severity = j.value("severity", v.severity);
  v.explanation = j.value("explanation", v.explanation);
  v.detected_at_epoch = j.value("detected_at_epoch", v.detected_at_epoch);
}

void to_json(nlohmann::json& j, const CashHealthScore& v) {
  j = {{"scope_type", v.scope_type}, {"scope_id", v.scope_id}, {"score", v.score},
       {"factors", v.factors}, {"explanations", v.explanations}};
}

void from_json(const nlohmann::json& j, CashHealthScore& v) {
  v.scope_type = j.value("scope_type", v.scope_type);
  v.scope_id = j.value("scope_id", v.scope_id);
  v.score = j.value("score", v.score);
  v.factors = j.value("factors", v.factors);
  v.explanations = j.value("explanations", v.explanations);
}

void to_json(nlohmann::json& j, const CashDashboardView& v) {
  j = {{"scope_type", v.scope_type},
       {"scope_id", v.scope_id},
       {"expected_balance", v.expected_balance},
       {"observed_balance", v.observed_balance},
       {"variance", v.variance},
       {"shortage_risk", v.shortage_risk},
       {"surplus_risk", v.surplus_risk},
       {"variance_trend", v.variance_trend},
       {"anomaly_summary", v.anomaly_summary},
       {"replenishment_recommendations", v.replenishment_recommendations},
       {"health", v.health}};
}

void to_json(nlohmann::json& j, const CashEvent& v) {
  j = {{"event_id", v.event_id},
       {"scope_type", v.scope_type},
       {"scope_id", v.scope_id},
       {"device_id", v.device_id},
       {"merchant_id", v.merchant_id},
       {"branch_id", v.branch_id},
       {"region", v.region},
       {"type", v.type},
       {"occurred_at_epoch", v.occurred_at_epoch},
       {"metadata", v.metadata}};
}

void from_json(const nlohmann::json& j, CashEvent& v) {
  v.event_id = j.value("event_id", v.event_id);
  v.scope_type = j.value("scope_type", v.scope_type);
  v.scope_id = j.value("scope_id", v.scope_id);
  v.device_id = j.value("device_id", v.device_id);
  v.merchant_id = j.value("merchant_id", v.merchant_id);
  v.branch_id = j.value("branch_id", v.branch_id);
  v.region = j.value("region", v.region);
  v.type = j.value("type", v.type);
  v.occurred_at_epoch = j.value("occurred_at_epoch", v.occurred_at_epoch);
  v.metadata = j.value("metadata", nlohmann::json::object());
}

}  // namespace cloud::cash_intelligence
