#pragma once

#include <mutex>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

#include "cloud/cash_intelligence/models.hpp"
#include "cloud/fleet_control/control_plane.hpp"

namespace cloud::cash_intelligence {

class CashIntelligenceService {
 public:
  explicit CashIntelligenceService(std::string store_path = "",
                                   cloud::fleet_control::FleetControlPlane* fleet = nullptr);

  void configure_denominations(const std::string& currency, std::vector<int> denominations);
  bool validate_currency(const std::string& currency) const;
  bool validate_denominations(const std::string& currency, const DenominationBreakdown& breakdown) const;
  long denomination_value(const DenominationBreakdown& breakdown) const;

  bool add_ledger_entry(CashLedgerEntry entry);
  std::vector<CashLedgerEntry> ledger_entries(const CashFilter& filter = {}) const;

  bool update_inventory(DenominationInventory inventory);
  std::vector<DenominationInventory> inventories(const CashFilter& filter = {}) const;
  int denomination_drift(const std::string& currency, const DenominationBreakdown& expected,
                         const DenominationBreakdown& observed) const;
  DenominationBreakdown recommend_mix(const std::string& currency, long target_balance) const;

  ReconciliationReport reconcile(const std::string& scope_type, const std::string& scope_id,
                                 const std::string& currency, long observed_balance,
                                 long now_epoch);
  std::vector<ReconciliationReport> reports(const CashFilter& filter = {}) const;

  CashForecast forecast(const std::string& scope_type, const std::string& scope_id,
                        const std::string& currency, long now_epoch);
  std::vector<CashAnomaly> detect_anomalies(const std::string& scope_type,
                                            const std::string& scope_id, long now_epoch);
  std::vector<CashAnomaly> anomalies(const CashFilter& filter = {}) const;

  CashHealthScore health_score(const std::string& scope_type, const std::string& scope_id);
  CashDashboardView dashboard(const std::string& scope_type, const std::string& scope_id,
                              const std::string& currency);
  std::vector<CashEvent> events(const CashFilter& filter = {}) const;

  void save() const;
  void load();

 private:
  bool scope_matches(const CashLedgerEntry& entry, const std::string& scope_type,
                     const std::string& scope_id) const;
  bool filter_matches(const CashLedgerEntry& entry, const CashFilter& filter) const;
  std::string org_for_entry_locked(const CashLedgerEntry& entry) const;
  void event_locked(const CashLedgerEntry& entry, const std::string& type, nlohmann::json metadata = {});
  void event_locked(const std::string& scope_type, const std::string& scope_id,
                    const std::string& device_id, const std::string& merchant_id,
                    const std::string& branch_id, const std::string& region,
                    const std::string& type, long now_epoch, nlohmann::json metadata = {});
  CashHealthScore health_score_locked(const std::string& scope_type, const std::string& scope_id);
  void save_locked() const;
  void publish_health_locked(const CashHealthScore& score) const;

  mutable std::mutex mu_;
  std::string store_path_;
  cloud::fleet_control::FleetControlPlane* fleet_{nullptr};
  std::vector<CashLedgerEntry> ledger_;
  std::vector<ReconciliationReport> reports_;
  std::vector<CashForecast> forecasts_;
  std::vector<CashAnomaly> anomalies_;
  std::vector<CashEvent> events_;
  std::unordered_map<std::string, DenominationInventory> inventories_;
  std::unordered_map<std::string, std::set<int>> denomination_sets_;
  std::set<std::string> seen_event_ids_;
};

CashIntelligenceService& default_cash_service();

}  // namespace cloud::cash_intelligence
