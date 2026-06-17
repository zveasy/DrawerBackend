#include "integrations/veil/routes.hpp"

#include <nlohmann/json.hpp>

#include "util/log.hpp"

namespace integrations::veil {
namespace {

void json_response(httplib::Response& res, const nlohmann::json& body, int status = 200) {
  res.status = status;
  res.set_content(body.dump(), "application/json");
}

std::string tenant_id(const httplib::Request& req) {
  auto tenant = req.get_header_value("X-Org-Id");
  if (tenant.empty()) tenant = req.get_param_value("tenant_id");
  if (tenant.empty()) tenant = req.get_param_value("organization_id");
  return tenant;
}

EvidenceFilter filter_from(const httplib::Request& req) {
  EvidenceFilter filter;
  filter.tenant_id = tenant_id(req);
  filter.device_id = req.get_param_value("device_id");
  filter.merchant_id = req.get_param_value("merchant_id");
  filter.branch_id = req.get_param_value("branch_id");
  filter.region = req.get_param_value("region");
  filter.event_type = req.get_param_value("event_type");
  if (req.has_param("start_time")) filter.start_time = std::stol(req.get_param_value("start_time"));
  if (req.has_param("end_time")) filter.end_time = std::stol(req.get_param_value("end_time"));
  return filter;
}

bool tenant_allowed(const httplib::Request& req, const std::string& tenant) {
  auto scoped = tenant_id(req);
  return scoped.empty() || tenant.empty() || scoped == tenant;
}

}  // namespace

void register_trust_routes(httplib::Server& server, VeilTrustService& service,
                           AccessCheck access_check) {
  auto allowed = [access_check](const httplib::Request& req, httplib::Response& res) {
    if (access_check && !access_check(req, res)) return false;
    util::log(util::LogLevel::Info, "trust_api_access",
              {{"src", req.remote_addr}, {"route", req.path}});
    return true;
  };

  server.Post("/trust/v1/evidence", [&service, allowed](const auto& req, auto& res) {
    if (!allowed(req, res)) return;
    try {
      auto evidence = nlohmann::json::parse(req.body).template get<TrustEvidenceRecord>();
      if (!tenant_allowed(req, evidence.tenant_id)) return json_response(res, {{"error", "tenant_forbidden"}}, 403);
      auto created = service.create_evidence(evidence);
      json_response(res, {{"evidence", created}});
    } catch (...) {
      json_response(res, {{"error", "bad_request"}}, 400);
    }
  });

  server.Get("/trust/v1/evidence", [&service, allowed](const auto& req, auto& res) {
    if (!allowed(req, res)) return;
    try {
      json_response(res, {{"evidence", service.list_evidence(filter_from(req))}});
    } catch (...) {
      json_response(res, {{"error", "bad_request"}}, 400);
    }
  });

  server.Post(R"(/trust/v1/evidence/([^/]+)/submit)", [&service, allowed](const auto& req, auto& res) {
    if (!allowed(req, res)) return;
    auto ok = service.submit_evidence(req.matches[1]);
    json_response(res, {{"ok", ok}}, ok ? 200 : 404);
  });

  server.Get("/trust/v1/verify", [&service, allowed](const auto& req, auto& res) {
    if (!allowed(req, res)) return;
    try {
      json_response(res, service.verify_chain(filter_from(req)));
    } catch (...) {
      json_response(res, {{"error", "bad_request"}}, 400);
    }
  });

  server.Get(R"(/trust/v1/export/([^/]+)/([^/]+))", [&service, allowed](const auto& req, auto& res) {
    if (!allowed(req, res)) return;
    try {
      long start = req.has_param("start_time") ? std::stol(req.get_param_value("start_time")) : 0;
      long end = req.has_param("end_time") ? std::stol(req.get_param_value("end_time")) : 0;
      auto bundle = service.export_bundle(req.matches[1], req.matches[2], start, end);
      json_response(res, bundle);
    } catch (...) {
      json_response(res, {{"error", "bad_request"}}, 400);
    }
  });

  server.Post("/trust/v1/policy/preview", [&service, allowed](const auto& req, auto& res) {
    if (!allowed(req, res)) return;
    try {
      auto j = nlohmann::json::parse(req.body);
      auto tenant = j.value("tenant_id", tenant_id(req));
      if (!tenant_allowed(req, tenant)) return json_response(res, {{"error", "tenant_forbidden"}}, 403);
      auto decision = service.verify_policy(tenant, j.value("device_id", ""), j.value("action", ""),
                                            j.value("context", nlohmann::json::object()),
                                            j.value("now_epoch", 0L));
      json_response(res, {{"decision", decision}});
    } catch (...) {
      json_response(res, {{"error", "bad_request"}}, 400);
    }
  });

  server.Get(R"(/trust/v1/score/([^/]+))", [&service, allowed](const auto& req, auto& res) {
    if (!allowed(req, res)) return;
    auto tenant = tenant_id(req);
    json_response(res, service.trust_score(tenant, req.matches[1], 0));
  });
}

}  // namespace integrations::veil
