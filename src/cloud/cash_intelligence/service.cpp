#include "cloud/cash_intelligence/service.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <numeric>

#include "cloud/fleet_control/routes.hpp"
#include "obs/metrics.hpp"

namespace cloud::cash_intelligence {
namespace {

std::string id_for(const std::string& prefix, size_t n) {
  return prefix + "-" + std::to_string(n + 1);
}

std::string inventory_key(const std::string& scope_type, const std::string& scope_id,
                          const std::string& currency) {
  return scope_type + ":" + scope_id + ":" + currency;
}

bool currency_code_ok(const std::string& currency) {
  if (currency.size() != 3) return false;
  return std::all_of(currency.begin(), currency.end(), [](unsigned char c) {
    return c >= 'A' && c <= 'Z';
  });
}

int severity_weight(const std::string& severity) {
  if (severity == "critical") return 25;
  if (severity == "warning") return 10;
  return 4;
}

std::string risk_from_variance(long variance) {
  if (variance < -10000) return "high";
  if (variance < -1000) return "medium";
  return "low";
}

long abs_long(long value) {
  return value < 0 ? -value : value;
}

}  // namespace

CashIntelligenceService::CashIntelligenceService(std::string store_path,
                                                 cloud::fleet_control::FleetControlPlane* fleet)
    : store_path_(std::move(store_path)), fleet_(fleet) {
  denomination_sets_["USD"] = {1, 5, 10, 25, 100, 500, 1000, 2000, 5000, 10000};
  denomination_sets_["KES"] = {50, 100, 500, 1000, 5000, 10000};
  denomination_sets_["NGN"] = {500, 1000, 2000, 5000, 10000, 20000, 50000, 100000};
  load();
}

void CashIntelligenceService::configure_denominations(const std::string& currency,
                                                      std::vector<int> denominations) {
  std::lock_guard<std::mutex> lk(mu_);
  denomination_sets_[currency] = std::set<int>(denominations.begin(), denominations.end());
  save_locked();
}

bool CashIntelligenceService::validate_currency(const std::string& currency) const {
  return currency_code_ok(currency);
}

bool CashIntelligenceService::validate_denominations(const std::string& currency,
                                                     const DenominationBreakdown& breakdown) const {
  if (!validate_currency(currency)) return false;
  auto set_it = denomination_sets_.find(currency);
  for (const auto& item : breakdown) {
    int denom = 0;
    try {
      denom = std::stoi(item.first);
    } catch (...) {
      return false;
    }
    if (item.second < 0) return false;
    if (set_it != denomination_sets_.end() && set_it->second.count(denom) == 0) return false;
  }
  return true;
}

long CashIntelligenceService::denomination_value(const DenominationBreakdown& breakdown) const {
  long total = 0;
  for (const auto& item : breakdown) {
    try {
      total += static_cast<long>(std::stoi(item.first)) * item.second;
    } catch (...) {
      return -1;
    }
  }
  return total;
}

bool CashIntelligenceService::add_ledger_entry(CashLedgerEntry entry) {
  if (entry.device_id.empty() || entry.currency.empty() ||
      !validate_denominations(entry.currency, entry.denominations)) {
    return false;
  }
  std::lock_guard<std::mutex> lk(mu_);
  if (entry.ledger_entry_id.empty()) entry.ledger_entry_id = id_for("cash-ledger", ledger_.size());
  if (entry.audit_event_id.empty()) entry.audit_event_id = id_for("cash-audit", events_.size());
  if (!entry.event_id.empty() && seen_event_ids_.count(entry.event_id)) entry.duplicate = true;
  if (!entry.event_id.empty()) seen_event_ids_.insert(entry.event_id);
  entry.variance = entry.observed_balance - entry.expected_balance;
  ledger_.push_back(entry);
  event_locked(entry, "cash_ledger_entry_created", {{"ledger_entry_id", entry.ledger_entry_id}});
  save_locked();
  return true;
}

bool CashIntelligenceService::filter_matches(const CashLedgerEntry& entry, const CashFilter& filter) const {
  if (!filter.device_id.empty() && entry.device_id != filter.device_id) return false;
  if (!filter.merchant_id.empty() && entry.merchant_id != filter.merchant_id) return false;
  if (!filter.branch_id.empty() && entry.branch_id != filter.branch_id) return false;
  if (!filter.region.empty() && entry.region != filter.region) return false;
  if (!filter.currency.empty() && entry.currency != filter.currency) return false;
  if (!filter.organization_id.empty()) {
    if (!fleet_) return false;
    auto device = fleet_->device(entry.device_id);
    if (!device || device->organization_id != filter.organization_id) return false;
  }
  return true;
}

std::vector<CashLedgerEntry> CashIntelligenceService::ledger_entries(const CashFilter& filter) const {
  std::lock_guard<std::mutex> lk(mu_);
  std::vector<CashLedgerEntry> out;
  for (const auto& entry : ledger_) {
    if (filter_matches(entry, filter)) out.push_back(entry);
  }
  return out;
}

bool CashIntelligenceService::update_inventory(DenominationInventory inventory) {
  if (!validate_denominations(inventory.currency, inventory.quantities) || inventory.scope_id.empty()) return false;
  std::lock_guard<std::mutex> lk(mu_);
  inventories_[inventory_key(inventory.scope_type, inventory.scope_id, inventory.currency)] = inventory;
  event_locked(inventory.scope_type, inventory.scope_id, inventory.scope_type == "device" ? inventory.scope_id : "",
               "", "", "", "replenishment_recommendation_created", inventory.updated_at_epoch,
               {{"inventory_updated", true}});
  save_locked();
  return true;
}

std::vector<DenominationInventory> CashIntelligenceService::inventories(const CashFilter& filter) const {
  std::lock_guard<std::mutex> lk(mu_);
  std::vector<DenominationInventory> out;
  for (const auto& item : inventories_) {
    const auto& inv = item.second;
    if (!filter.currency.empty() && inv.currency != filter.currency) continue;
    if (!filter.device_id.empty() && !(inv.scope_type == "device" && inv.scope_id == filter.device_id)) continue;
    if (!filter.branch_id.empty() && !(inv.scope_type == "branch" && inv.scope_id == filter.branch_id)) continue;
    if (!filter.merchant_id.empty() && !(inv.scope_type == "merchant" && inv.scope_id == filter.merchant_id)) continue;
    if (!filter.region.empty() && !(inv.scope_type == "region" && inv.scope_id == filter.region)) continue;
    out.push_back(inv);
  }
  return out;
}

int CashIntelligenceService::denomination_drift(const std::string& currency,
                                                const DenominationBreakdown& expected,
                                                const DenominationBreakdown& observed) const {
  if (!validate_denominations(currency, expected) || !validate_denominations(currency, observed)) return -1;
  int drift = 0;
  std::set<std::string> keys;
  for (const auto& item : expected) keys.insert(item.first);
  for (const auto& item : observed) keys.insert(item.first);
  for (const auto& key : keys) {
    int left = expected.count(key) ? expected.at(key) : 0;
    int right = observed.count(key) ? observed.at(key) : 0;
    drift += std::abs(left - right);
  }
  return drift;
}

DenominationBreakdown CashIntelligenceService::recommend_mix(const std::string& currency,
                                                             long target_balance) const {
  DenominationBreakdown mix;
  auto it = denomination_sets_.find(currency);
  if (target_balance <= 0 || it == denomination_sets_.end()) return mix;
  std::vector<int> denoms(it->second.begin(), it->second.end());
  std::sort(denoms.rbegin(), denoms.rend());
  long remaining = target_balance;
  for (int denom : denoms) {
    int qty = static_cast<int>(remaining / denom);
    if (qty > 0) {
      mix[std::to_string(denom)] = qty;
      remaining -= static_cast<long>(qty) * denom;
    }
  }
  return mix;
}

bool CashIntelligenceService::scope_matches(const CashLedgerEntry& entry, const std::string& scope_type,
                                            const std::string& scope_id) const {
  if (scope_type == "device") return entry.device_id == scope_id;
  if (scope_type == "branch") return entry.branch_id == scope_id;
  if (scope_type == "merchant") return entry.merchant_id == scope_id;
  if (scope_type == "region") return entry.region == scope_id;
  return false;
}

ReconciliationReport CashIntelligenceService::reconcile(const std::string& scope_type,
                                                        const std::string& scope_id,
                                                        const std::string& currency,
                                                        long observed_balance, long now_epoch) {
  std::lock_guard<std::mutex> lk(mu_);
  ReconciliationReport report;
  report.report_id = id_for("recon", reports_.size());
  report.scope_type = scope_type;
  report.scope_id = scope_id;
  report.currency = currency;
  report.observed_balance = observed_balance;
  report.started_at_epoch = now_epoch;
  report.completed_at_epoch = now_epoch;
  report.audit_event_id = id_for("cash-audit", events_.size());
  event_locked(scope_type, scope_id, scope_type == "device" ? scope_id : "", "", "", "",
               "reconciliation_started", now_epoch);

  std::set<std::string> seen;
  bool duplicate = false;
  bool drawer_open = false;
  bool stale_inventory = false;
  int matched_entries = 0;
  for (const auto& entry : ledger_) {
    if (!scope_matches(entry, scope_type, scope_id) || entry.currency != currency) continue;
    ++matched_entries;
    report.expected_balance += entry.expected_balance;
    if (report.merchant_id.empty()) report.merchant_id = entry.merchant_id;
    if (report.branch_id.empty()) report.branch_id = entry.branch_id;
    if (report.region.empty()) report.region = entry.region;
    if (!entry.event_id.empty() && !seen.insert(entry.event_id).second) duplicate = true;
    if (entry.duplicate) duplicate = true;
    if (entry.event_type == "drawer_open") drawer_open = true;
    if (entry.event_type == "inventory_snapshot" && now_epoch - entry.occurred_at_epoch > 86400) {
      stale_inventory = true;
    }
  }
  report.variance = report.observed_balance - report.expected_balance;
  long abs_variance = abs_long(report.variance);
  if (abs_variance == 0 && !duplicate && !drawer_open && !stale_inventory) {
    report.status = "balanced";
  } else if (duplicate) {
    report.status = "unresolved";
    report.findings.push_back("duplicate_events");
  } else if (drawer_open && abs_variance > 0) {
    report.status = "suspected_fraud";
    report.findings.push_back("unexplained_drawer_open");
  } else if (report.variance < -5000) {
    report.status = "suspected_loss";
    report.findings.push_back("shortage");
  } else if (abs_variance > 10000) {
    report.status = "major_variance";
  } else if (abs_variance > 0) {
    report.status = "minor_variance";
  }
  if (stale_inventory) report.findings.push_back("stale_inventory");
  if (matched_entries == 0) {
    report.status = "unresolved";
    report.findings.push_back("missing_events");
  }
  if (abs_variance > 0) {
    event_locked(scope_type, scope_id, scope_type == "device" ? scope_id : "", report.merchant_id,
                 report.branch_id, report.region, "reconciliation_variance_detected", now_epoch,
                 {{"variance", report.variance}});
  }
  reports_.push_back(report);
  event_locked(scope_type, scope_id, scope_type == "device" ? scope_id : "", report.merchant_id,
               report.branch_id, report.region, "reconciliation_completed", now_epoch,
               {{"report_id", report.report_id}, {"status", report.status}});
  save_locked();
  return report;
}

std::vector<ReconciliationReport> CashIntelligenceService::reports(const CashFilter& filter) const {
  std::lock_guard<std::mutex> lk(mu_);
  std::vector<ReconciliationReport> out;
  for (const auto& report : reports_) {
    if (!filter.currency.empty() && report.currency != filter.currency) continue;
    if (!filter.merchant_id.empty() && report.merchant_id != filter.merchant_id) continue;
    if (!filter.branch_id.empty() && report.branch_id != filter.branch_id) continue;
    if (!filter.region.empty() && report.region != filter.region) continue;
    if (!filter.device_id.empty() && !(report.scope_type == "device" && report.scope_id == filter.device_id)) continue;
    out.push_back(report);
  }
  return out;
}

CashForecast CashIntelligenceService::forecast(const std::string& scope_type, const std::string& scope_id,
                                               const std::string& currency, long now_epoch) {
  std::lock_guard<std::mutex> lk(mu_);
  CashForecast fc;
  fc.forecast_id = id_for("cash-forecast", forecasts_.size());
  fc.scope_type = scope_type;
  fc.scope_id = scope_id;
  fc.currency = currency;
  long net = 0;
  int count = 0;
  long latest_observed = 0;
  long first_time = 0, last_time = 0;
  for (const auto& entry : ledger_) {
    if (!scope_matches(entry, scope_type, scope_id) || entry.currency != currency) continue;
    net += entry.observed_balance - entry.expected_balance;
    latest_observed = entry.observed_balance;
    if (first_time == 0 || entry.occurred_at_epoch < first_time) first_time = entry.occurred_at_epoch;
    if (entry.occurred_at_epoch > last_time) last_time = entry.occurred_at_epoch;
    ++count;
  }
  long elapsed_hours = std::max(1L, (last_time - first_time) / 3600);
  long hourly_change = count ? net / elapsed_hours : 0;
  if (hourly_change < 0 && latest_observed > 0) {
    fc.expected_depletion_epoch = now_epoch + (latest_observed / std::max(1L, -hourly_change)) * 3600;
    fc.recommended_replenishment_epoch = std::max(now_epoch, fc.expected_depletion_epoch - 12 * 3600);
    fc.high_risk_shortage_windows.push_back(std::to_string(fc.recommended_replenishment_epoch) + "-" +
                                            std::to_string(fc.expected_depletion_epoch));
  } else if (hourly_change > 0) {
    fc.expected_surplus_epoch = now_epoch + 24 * 3600;
  }
  fc.confidence = count >= 5 ? "high" : count >= 2 ? "medium" : "low";
  fc.explanation = "deterministic hourly cash movement baseline";
  forecasts_.push_back(fc);
  event_locked(scope_type, scope_id, scope_type == "device" ? scope_id : "", "", "", "",
               "cash_forecast_generated", now_epoch, {{"forecast_id", fc.forecast_id}});
  save_locked();
  return fc;
}

std::vector<CashAnomaly> CashIntelligenceService::detect_anomalies(const std::string& scope_type,
                                                                   const std::string& scope_id,
                                                                   long now_epoch) {
  std::lock_guard<std::mutex> lk(mu_);
  int small_shortages = 0;
  int manual_adjustments = 0;
  int cash_out = 0;
  std::vector<CashAnomaly> created;
  for (const auto& entry : ledger_) {
    if (!scope_matches(entry, scope_type, scope_id)) continue;
    if (entry.variance < 0 && entry.variance > -1000) ++small_shortages;
    if (entry.variance <= -10000) {
      CashAnomaly a{id_for("cash-anomaly", anomalies_.size() + created.size()), scope_type, scope_id,
                    entry.device_id, entry.merchant_id, entry.branch_id, entry.region,
                    "large_one_time_variance", "critical",
                    "observed cash is materially below expected cash", now_epoch};
      created.push_back(a);
    }
    if (entry.event_type == "drawer_open" && (entry.occurred_at_epoch % 86400 < 6 * 3600 ||
                                              entry.occurred_at_epoch % 86400 > 22 * 3600)) {
      created.push_back({id_for("cash-anomaly", anomalies_.size() + created.size()), scope_type, scope_id,
                         entry.device_id, entry.merchant_id, entry.branch_id, entry.region,
                         "after_hours_drawer_activity", "warning",
                         "drawer opened outside configured operating window", now_epoch});
    }
    if (entry.event_type == "manual_adjustment") ++manual_adjustments;
    if (entry.event_type == "cash_out") ++cash_out;
  }
  if (small_shortages >= 3) {
    created.push_back({id_for("cash-anomaly", anomalies_.size() + created.size()), scope_type, scope_id,
                       "", "", "", "", "repeated_small_shortages", "warning",
                       "multiple small negative variances detected", now_epoch});
  }
  if (manual_adjustments > 3) {
    created.push_back({id_for("cash-anomaly", anomalies_.size() + created.size()), scope_type, scope_id,
                       "", "", "", "", "excessive_manual_adjustments", "warning",
                       "manual adjustments exceed pilot baseline", now_epoch});
  }
  if (cash_out > 5) {
    created.push_back({id_for("cash-anomaly", anomalies_.size() + created.size()), scope_type, scope_id,
                       "", "", "", "", "unusual_cash_out_frequency", "warning",
                       "cash-out count exceeds deterministic threshold", now_epoch});
  }
  for (const auto& anomaly : created) {
    anomalies_.push_back(anomaly);
    event_locked(scope_type, scope_id, anomaly.device_id, anomaly.merchant_id, anomaly.branch_id,
                 anomaly.region, "cash_anomaly_detected", now_epoch,
                 {{"type", anomaly.type}, {"severity", anomaly.severity}});
    if (fleet_) {
      std::string org = "";
      if (!anomaly.device_id.empty()) {
        auto device = fleet_->device(anomaly.device_id);
        if (device) org = device->organization_id;
      }
      fleet_->create_alert(org, anomaly.device_id, anomaly.type, anomaly.severity,
                           anomaly.explanation, now_epoch);
    }
  }
  save_locked();
  return created;
}

std::vector<CashAnomaly> CashIntelligenceService::anomalies(const CashFilter& filter) const {
  std::lock_guard<std::mutex> lk(mu_);
  std::vector<CashAnomaly> out;
  for (const auto& anomaly : anomalies_) {
    if (!filter.device_id.empty() && anomaly.device_id != filter.device_id) continue;
    if (!filter.merchant_id.empty() && anomaly.merchant_id != filter.merchant_id) continue;
    if (!filter.branch_id.empty() && anomaly.branch_id != filter.branch_id) continue;
    if (!filter.region.empty() && anomaly.region != filter.region) continue;
    out.push_back(anomaly);
  }
  return out;
}

CashHealthScore CashIntelligenceService::health_score(const std::string& scope_type,
                                                      const std::string& scope_id) {
  std::lock_guard<std::mutex> lk(mu_);
  return health_score_locked(scope_type, scope_id);
}

CashHealthScore CashIntelligenceService::health_score_locked(const std::string& scope_type,
                                                             const std::string& scope_id) {
  CashHealthScore score;
  score.scope_type = scope_type;
  score.scope_id = scope_id;
  long worst_variance = 0;
  int stale_inventory = 0;
  for (const auto& report : reports_) {
    if (report.scope_type == scope_type && report.scope_id == scope_id) {
      worst_variance = std::max(worst_variance, abs_long(report.variance));
    }
  }
  for (const auto& item : inventories_) {
    const auto& inv = item.second;
    if (inv.scope_type == scope_type && inv.scope_id == scope_id && inv.updated_at_epoch > 0) {
      auto newest_event = std::max_element(events_.begin(), events_.end(), [](const auto& a, const auto& b) {
        return a.occurred_at_epoch < b.occurred_at_epoch;
      });
      if (newest_event != events_.end() && newest_event->occurred_at_epoch - inv.updated_at_epoch > 86400) {
        ++stale_inventory;
      }
    }
  }
  int anomaly_penalty = 0;
  for (const auto& anomaly : anomalies_) {
    if (anomaly.scope_type == scope_type && anomaly.scope_id == scope_id) {
      anomaly_penalty += severity_weight(anomaly.severity);
    }
  }
  score.factors["variance"] = static_cast<int>(std::min(40L, worst_variance / 1000));
  score.factors["stale_inventory"] = stale_inventory * 10;
  score.factors["anomalies"] = anomaly_penalty;
  score.factors["sync_health"] = 0;
  score.factors["transaction_failures"] = 0;
  score.factors["offline_duration"] = 0;
  int total_penalty = 0;
  for (const auto& factor : score.factors) total_penalty += factor.second;
  score.score = std::max(0, 100 - total_penalty);
  if (score.factors["variance"] > 0) score.explanations.push_back("reconciliation variance reduced score");
  if (score.factors["stale_inventory"] > 0) score.explanations.push_back("stale inventory reduced score");
  if (score.factors["anomalies"] > 0) score.explanations.push_back("cash anomalies reduced score");
  publish_health_locked(score);
  event_locked(scope_type, scope_id, scope_type == "device" ? scope_id : "", "", "", "",
               "cash_health_score_updated", 0, {{"score", score.score}});
  save_locked();
  return score;
}

CashDashboardView CashIntelligenceService::dashboard(const std::string& scope_type,
                                                     const std::string& scope_id,
                                                     const std::string& currency) {
  std::lock_guard<std::mutex> lk(mu_);
  CashDashboardView view;
  view.scope_type = scope_type;
  view.scope_id = scope_id;
  for (const auto& entry : ledger_) {
    if (!scope_matches(entry, scope_type, scope_id) || (!currency.empty() && entry.currency != currency)) continue;
    view.expected_balance += entry.expected_balance;
    view.observed_balance += entry.observed_balance;
    view.variance += entry.variance;
  }
  for (const auto& report : reports_) {
    if (report.scope_type == scope_type && report.scope_id == scope_id &&
        (currency.empty() || report.currency == currency)) {
      view.variance_trend.push_back(report.variance);
    }
  }
  for (const auto& anomaly : anomalies_) {
    if (anomaly.scope_type == scope_type && anomaly.scope_id == scope_id) ++view.anomaly_summary[anomaly.severity];
  }
  view.shortage_risk = risk_from_variance(view.variance);
  view.surplus_risk = view.variance > 10000 ? "high" : view.variance > 1000 ? "medium" : "low";
  auto mix = recommend_mix(currency.empty() ? "USD" : currency, std::max(0L, -view.variance + 10000));
  if (!mix.empty()) view.replenishment_recommendations.push_back(nlohmann::json(mix).dump());
  view.health = health_score_locked(scope_type, scope_id);
  return view;
}

std::vector<CashEvent> CashIntelligenceService::events(const CashFilter& filter) const {
  std::lock_guard<std::mutex> lk(mu_);
  std::vector<CashEvent> out;
  for (const auto& event : events_) {
    if (!filter.device_id.empty() && event.device_id != filter.device_id) continue;
    if (!filter.merchant_id.empty() && event.merchant_id != filter.merchant_id) continue;
    if (!filter.branch_id.empty() && event.branch_id != filter.branch_id) continue;
    if (!filter.region.empty() && event.region != filter.region) continue;
    out.push_back(event);
  }
  return out;
}

void CashIntelligenceService::save() const {
  std::lock_guard<std::mutex> lk(mu_);
  save_locked();
}

void CashIntelligenceService::load() {
  if (store_path_.empty()) return;
  std::ifstream in(store_path_);
  if (!in) return;
  nlohmann::json j;
  in >> j;
  ledger_ = j.value("ledger", ledger_);
  reports_ = j.value("reports", reports_);
  forecasts_ = j.value("forecasts", forecasts_);
  anomalies_ = j.value("anomalies", anomalies_);
  events_ = j.value("events", events_);
  for (const auto& item : j.value("inventories", nlohmann::json::array())) {
    auto inv = item.get<DenominationInventory>();
    inventories_[inventory_key(inv.scope_type, inv.scope_id, inv.currency)] = inv;
  }
  for (const auto& item : j.value("denomination_sets", nlohmann::json::object()).items()) {
    auto values = item.value().get<std::vector<int>>();
    denomination_sets_[item.key()] = std::set<int>(values.begin(), values.end());
  }
  for (const auto& entry : ledger_) {
    if (!entry.event_id.empty()) seen_event_ids_.insert(entry.event_id);
  }
}

void CashIntelligenceService::save_locked() const {
  if (store_path_.empty()) return;
  auto parent = std::filesystem::path(store_path_).parent_path();
  if (!parent.empty()) std::filesystem::create_directories(parent);
  nlohmann::json inv = nlohmann::json::array();
  for (const auto& item : inventories_) inv.push_back(item.second);
  nlohmann::json sets = nlohmann::json::object();
  for (const auto& item : denomination_sets_) {
    sets[item.first] = std::vector<int>(item.second.begin(), item.second.end());
  }
  std::ofstream(store_path_, std::ios::trunc)
      << nlohmann::json{{"schema", 1},
                        {"ledger", ledger_},
                        {"reports", reports_},
                        {"forecasts", forecasts_},
                        {"anomalies", anomalies_},
                        {"events", events_},
                        {"inventories", inv},
                        {"denomination_sets", sets}}
             .dump(2)
      << "\n";
}

void CashIntelligenceService::event_locked(const CashLedgerEntry& entry, const std::string& type,
                                           nlohmann::json metadata) {
  event_locked("device", entry.device_id, entry.device_id, entry.merchant_id, entry.branch_id,
               entry.region, type, entry.occurred_at_epoch, std::move(metadata));
}

void CashIntelligenceService::event_locked(const std::string& scope_type, const std::string& scope_id,
                                           const std::string& device_id, const std::string& merchant_id,
                                           const std::string& branch_id, const std::string& region,
                                           const std::string& type, long now_epoch,
                                           nlohmann::json metadata) {
  events_.push_back({id_for("cash-event", events_.size()), scope_type, scope_id, device_id,
                     merchant_id, branch_id, region, type, now_epoch, std::move(metadata)});
}

void CashIntelligenceService::publish_health_locked(const CashHealthScore& score) const {
  obs::M().gauge("register_cash_health_score", "Cash health score",
                 {{"scope_type", score.scope_type}, {"scope_id", score.scope_id}})
      .set(score.score);
}

CashIntelligenceService& default_cash_service() {
  static CashIntelligenceService* svc = [] {
    const char* env = std::getenv("REGISTER_MVP_CASH_STORE");
    std::string path = env && *env ? std::string(env) : "data/cash_intelligence.json";
    return new CashIntelligenceService(path, &cloud::fleet_control::default_control_plane());
  }();
  return *svc;
}

}  // namespace cloud::cash_intelligence
