#include "cloud/cash_intelligence/routes.hpp"

#include <nlohmann/json.hpp>

#include "cloud/fleet_control/routes.hpp"
#include "util/log.hpp"

namespace cloud::cash_intelligence {
namespace {

void json_response(httplib::Response& res, const nlohmann::json& body, int status = 200) {
  res.status = status;
  res.set_content(body.dump(), "application/json");
}

std::string tenant_org(const httplib::Request& req) {
  auto org = req.get_header_value("X-Org-Id");
  if (org.empty()) org = req.get_param_value("organization_id");
  return org;
}

CashFilter filter_from(const httplib::Request& req) {
  CashFilter filter;
  filter.organization_id = tenant_org(req);
  filter.device_id = req.get_param_value("device_id");
  filter.merchant_id = req.get_param_value("merchant_id");
  filter.branch_id = req.get_param_value("branch_id");
  filter.region = req.get_param_value("region");
  filter.currency = req.get_param_value("currency");
  return filter;
}

bool tenant_allows_device(const httplib::Request& req, const std::string& device_id) {
  auto org = tenant_org(req);
  if (org.empty() || device_id.empty()) return true;
  auto device = cloud::fleet_control::default_control_plane().device(device_id);
  return device && device->organization_id == org;
}

}  // namespace

void register_cash_intelligence_routes(httplib::Server& server, CashIntelligenceService& service,
                                       AccessCheck access_check) {
  auto allowed = [access_check](const httplib::Request& req, httplib::Response& res) {
    if (access_check && !access_check(req, res)) return false;
    util::log(util::LogLevel::Info, "cash_intelligence_api_access",
              {{"src", req.remote_addr}, {"route", req.path}});
    return true;
  };

  server.Get("/cash/v1/ledger", [&service, allowed](const auto& req, auto& res) {
    if (!allowed(req, res)) return;
    json_response(res, {{"entries", service.ledger_entries(filter_from(req))}});
  });

  server.Post("/cash/v1/ledger", [&service, allowed](const auto& req, auto& res) {
    if (!allowed(req, res)) return;
    try {
      auto entry = nlohmann::json::parse(req.body).template get<CashLedgerEntry>();
      if (!tenant_allows_device(req, entry.device_id)) return json_response(res, {{"error", "tenant_forbidden"}}, 403);
      auto ok = service.add_ledger_entry(entry);
      json_response(res, {{"ok", ok}, {"entry", entry}}, ok ? 200 : 400);
    } catch (...) {
      json_response(res, {{"error", "bad_request"}}, 400);
    }
  });

  server.Get("/cash/v1/inventory", [&service, allowed](const auto& req, auto& res) {
    if (!allowed(req, res)) return;
    json_response(res, {{"inventories", service.inventories(filter_from(req))}});
  });

  server.Post("/cash/v1/inventory", [&service, allowed](const auto& req, auto& res) {
    if (!allowed(req, res)) return;
    try {
      auto inv = nlohmann::json::parse(req.body).template get<DenominationInventory>();
      auto ok = service.update_inventory(inv);
      json_response(res, {{"ok", ok}, {"inventory", inv}}, ok ? 200 : 400);
    } catch (...) {
      json_response(res, {{"error", "bad_request"}}, 400);
    }
  });

  server.Post("/cash/v1/reconcile", [&service, allowed](const auto& req, auto& res) {
    if (!allowed(req, res)) return;
    try {
      auto j = nlohmann::json::parse(req.body);
      auto report = service.reconcile(j.value("scope_type", "device"), j.value("scope_id", ""),
                                      j.value("currency", ""), j.value("observed_balance", 0L),
                                      j.value("now_epoch", 0L));
      json_response(res, {{"report", report}});
    } catch (...) {
      json_response(res, {{"error", "bad_request"}}, 400);
    }
  });

  server.Get("/cash/v1/reconciliation", [&service, allowed](const auto& req, auto& res) {
    if (!allowed(req, res)) return;
    json_response(res, {{"reports", service.reports(filter_from(req))}});
  });

  server.Post("/cash/v1/forecast", [&service, allowed](const auto& req, auto& res) {
    if (!allowed(req, res)) return;
    try {
      auto j = nlohmann::json::parse(req.body);
      auto fc = service.forecast(j.value("scope_type", "device"), j.value("scope_id", ""),
                                 j.value("currency", ""), j.value("now_epoch", 0L));
      json_response(res, {{"forecast", fc}});
    } catch (...) {
      json_response(res, {{"error", "bad_request"}}, 400);
    }
  });

  server.Get("/cash/v1/anomalies", [&service, allowed](const auto& req, auto& res) {
    if (!allowed(req, res)) return;
    json_response(res, {{"anomalies", service.anomalies(filter_from(req))}});
  });

  server.Post("/cash/v1/anomalies/detect", [&service, allowed](const auto& req, auto& res) {
    if (!allowed(req, res)) return;
    try {
      auto j = nlohmann::json::parse(req.body);
      auto anomalies = service.detect_anomalies(j.value("scope_type", "device"),
                                                j.value("scope_id", ""), j.value("now_epoch", 0L));
      json_response(res, {{"anomalies", anomalies}});
    } catch (...) {
      json_response(res, {{"error", "bad_request"}}, 400);
    }
  });

  server.Get(R"(/cash/v1/health/([^/]+)/([^/]+))", [&service, allowed](const auto& req, auto& res) {
    if (!allowed(req, res)) return;
    json_response(res, service.health_score(req.matches[1], req.matches[2]));
  });

  server.Get(R"(/cash/v1/dashboard/([^/]+)/([^/]+))", [&service, allowed](const auto& req, auto& res) {
    if (!allowed(req, res)) return;
    json_response(res, service.dashboard(req.matches[1], req.matches[2], req.get_param_value("currency")));
  });
}

}  // namespace cloud::cash_intelligence
