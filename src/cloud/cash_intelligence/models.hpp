#pragma once

#include <map>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

namespace cloud::cash_intelligence {

using DenominationBreakdown = std::map<std::string, int>;

struct CashLedgerEntry {
  std::string ledger_entry_id;
  std::string event_type;
  std::string event_id;
  std::string device_id;
  std::string merchant_id;
  std::string branch_id;
  std::string region;
  std::string currency;
  DenominationBreakdown denominations;
  long expected_balance{0};
  long observed_balance{0};
  long variance{0};
  std::string actor_id;
  std::string actor_role;
  long occurred_at_epoch{0};
  std::string audit_event_id;
  bool duplicate{false};
};

struct DenominationInventory {
  std::string scope_type{"device"};
  std::string scope_id;
  std::string currency;
  DenominationBreakdown quantities;
  long updated_at_epoch{0};
};

struct ReconciliationReport {
  std::string report_id;
  std::string scope_type;
  std::string scope_id;
  std::string merchant_id;
  std::string branch_id;
  std::string region;
  std::string currency;
  long expected_balance{0};
  long observed_balance{0};
  long variance{0};
  std::string status{"balanced"};
  std::vector<std::string> findings;
  long started_at_epoch{0};
  long completed_at_epoch{0};
  std::string audit_event_id;
};

struct CashForecast {
  std::string forecast_id;
  std::string scope_type;
  std::string scope_id;
  std::string currency;
  long expected_depletion_epoch{0};
  long expected_surplus_epoch{0};
  long recommended_replenishment_epoch{0};
  std::vector<std::string> high_risk_shortage_windows;
  std::string confidence{"low"};
  std::string explanation;
};

struct CashAnomaly {
  std::string anomaly_id;
  std::string scope_type;
  std::string scope_id;
  std::string device_id;
  std::string merchant_id;
  std::string branch_id;
  std::string region;
  std::string type;
  std::string severity{"warning"};
  std::string explanation;
  long detected_at_epoch{0};
};

struct CashHealthScore {
  std::string scope_type;
  std::string scope_id;
  int score{100};
  std::map<std::string, int> factors;
  std::vector<std::string> explanations;
};

struct CashDashboardView {
  std::string scope_type;
  std::string scope_id;
  long expected_balance{0};
  long observed_balance{0};
  long variance{0};
  std::string shortage_risk{"low"};
  std::string surplus_risk{"low"};
  std::vector<long> variance_trend;
  std::map<std::string, int> anomaly_summary;
  std::vector<std::string> replenishment_recommendations;
  CashHealthScore health;
};

struct CashEvent {
  std::string event_id;
  std::string scope_type;
  std::string scope_id;
  std::string device_id;
  std::string merchant_id;
  std::string branch_id;
  std::string region;
  std::string type;
  long occurred_at_epoch{0};
  nlohmann::json metadata = nlohmann::json::object();
};

struct CashFilter {
  std::string organization_id;
  std::string device_id;
  std::string merchant_id;
  std::string branch_id;
  std::string region;
  std::string currency;
};

void to_json(nlohmann::json& j, const CashLedgerEntry& v);
void from_json(const nlohmann::json& j, CashLedgerEntry& v);
void to_json(nlohmann::json& j, const DenominationInventory& v);
void from_json(const nlohmann::json& j, DenominationInventory& v);
void to_json(nlohmann::json& j, const ReconciliationReport& v);
void from_json(const nlohmann::json& j, ReconciliationReport& v);
void to_json(nlohmann::json& j, const CashForecast& v);
void from_json(const nlohmann::json& j, CashForecast& v);
void to_json(nlohmann::json& j, const CashAnomaly& v);
void from_json(const nlohmann::json& j, CashAnomaly& v);
void to_json(nlohmann::json& j, const CashHealthScore& v);
void from_json(const nlohmann::json& j, CashHealthScore& v);
void to_json(nlohmann::json& j, const CashDashboardView& v);
void to_json(nlohmann::json& j, const CashEvent& v);
void from_json(const nlohmann::json& j, CashEvent& v);

}  // namespace cloud::cash_intelligence
