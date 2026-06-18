#include "cloud/fleet_control/routes.hpp"

#include <cstdlib>

#include <nlohmann/json.hpp>

#include "util/log.hpp"

namespace cloud::fleet_control {
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

bool allowed_tenant(const httplib::Request& req, const std::string& org) {
  auto tenant = tenant_org(req);
  return tenant.empty() || org.empty() || tenant == org;
}

DeviceFilter filter_from(const httplib::Request& req) {
  DeviceFilter filter;
  filter.organization_id = tenant_org(req);
  filter.merchant_id = req.get_param_value("merchant_id");
  filter.branch_id = req.get_param_value("branch_id");
  filter.region = req.get_param_value("region");
  filter.environment = req.get_param_value("environment");
  filter.deployment_channel = req.get_param_value("deployment_channel");
  filter.health_status = req.get_param_value("health_status");
  filter.connectivity_status = req.get_param_value("connectivity_status");
  filter.tag = req.get_param_value("tag");
  filter.label_key = req.get_param_value("label_key");
  filter.label_value = req.get_param_value("label_value");
  filter.text = req.get_param_value("q");
  return filter;
}

}  // namespace

void register_fleet_control_routes(httplib::Server& server, FleetControlPlane& control,
                                   AccessCheck access_check) {
  auto allowed = [access_check](const httplib::Request& req, httplib::Response& res) {
    if (access_check && !access_check(req, res)) return false;
    util::log(util::LogLevel::Info, "fleet_control_api_access",
              {{"src", req.remote_addr}, {"route", req.path}});
    return true;
  };

  server.Get("/fleet-control/v1/organizations", [&control, allowed](const auto& req, auto& res) {
    if (!allowed(req, res)) return;
    json_response(res, {{"organizations", control.organizations(tenant_org(req))}});
  });
  server.Post("/fleet-control/v1/organizations", [&control, allowed](const auto& req, auto& res) {
    if (!allowed(req, res)) return;
    try {
      auto body = nlohmann::json::parse(req.body);
      auto org = body.template get<Organization>();
      if (!allowed_tenant(req, org.organization_id)) return json_response(res, {{"error", "tenant_forbidden"}}, 403);
      control.upsert_organization(org);
      json_response(res, {{"ok", true}, {"organization", org}});
    } catch (...) {
      json_response(res, {{"error", "bad_request"}}, 400);
    }
  });

  server.Get("/fleet-control/v1/merchants", [&control, allowed](const auto& req, auto& res) {
    if (!allowed(req, res)) return;
    json_response(res, {{"merchants", control.merchants(tenant_org(req))}});
  });
  server.Post("/fleet-control/v1/merchants", [&control, allowed](const auto& req, auto& res) {
    if (!allowed(req, res)) return;
    try {
      auto body = nlohmann::json::parse(req.body);
      auto merchant = body.template get<Merchant>();
      if (!allowed_tenant(req, merchant.organization_id)) return json_response(res, {{"error", "tenant_forbidden"}}, 403);
      control.upsert_merchant(merchant);
      json_response(res, {{"ok", true}, {"merchant", merchant}});
    } catch (...) {
      json_response(res, {{"error", "bad_request"}}, 400);
    }
  });

  server.Get("/fleet-control/v1/branches", [&control, allowed](const auto& req, auto& res) {
    if (!allowed(req, res)) return;
    json_response(res, {{"branches", control.branches(tenant_org(req))}});
  });
  server.Post("/fleet-control/v1/branches", [&control, allowed](const auto& req, auto& res) {
    if (!allowed(req, res)) return;
    try {
      auto body = nlohmann::json::parse(req.body);
      auto branch = body.template get<Branch>();
      if (!allowed_tenant(req, branch.organization_id)) return json_response(res, {{"error", "tenant_forbidden"}}, 403);
      control.upsert_branch(branch);
      json_response(res, {{"ok", true}, {"branch", branch}});
    } catch (...) {
      json_response(res, {{"error", "bad_request"}}, 400);
    }
  });

  server.Get("/fleet-control/v1/regions", [&control, allowed](const auto& req, auto& res) {
    if (!allowed(req, res)) return;
    json_response(res, {{"regions", control.regions(tenant_org(req))}});
  });
  server.Post("/fleet-control/v1/regions", [&control, allowed](const auto& req, auto& res) {
    if (!allowed(req, res)) return;
    try {
      auto body = nlohmann::json::parse(req.body);
      auto region = body.template get<Region>();
      if (!allowed_tenant(req, region.organization_id)) return json_response(res, {{"error", "tenant_forbidden"}}, 403);
      control.upsert_region(region);
      json_response(res, {{"ok", true}, {"region", region}});
    } catch (...) {
      json_response(res, {{"error", "bad_request"}}, 400);
    }
  });

  server.Get("/fleet-control/v1/devices", [&control, allowed](const auto& req, auto& res) {
    if (!allowed(req, res)) return;
    json_response(res, {{"devices", control.search_devices(filter_from(req))}});
  });
  server.Post("/fleet-control/v1/devices", [&control, allowed](const auto& req, auto& res) {
    if (!allowed(req, res)) return;
    try {
      auto body = nlohmann::json::parse(req.body);
      auto device = body.template get<DeviceRegistryRecord>();
      if (!allowed_tenant(req, device.organization_id)) return json_response(res, {{"error", "tenant_forbidden"}}, 403);
      json_response(res, {{"ok", control.register_device(device)}, {"device", device}});
    } catch (...) {
      json_response(res, {{"error", "bad_request"}}, 400);
    }
  });
  server.Post(R"(/fleet-control/v1/devices/([^/]+)/heartbeat)", [&control, allowed](const auto& req, auto& res) {
    if (!allowed(req, res)) return;
    try {
      auto body = nlohmann::json::parse(req.body);
      auto hb = body.template get<HeartbeatMessage>();
      hb.device_id = req.matches[1];
      auto device = control.device(hb.device_id);
      if (!device || !allowed_tenant(req, device->organization_id)) return json_response(res, {{"error", "not_found"}}, 404);
      json_response(res, {{"ok", control.receive_heartbeat(hb)}});
    } catch (...) {
      json_response(res, {{"error", "bad_request"}}, 400);
    }
  });

  server.Post(R"(/fleet-control/v1/devices/([^/]+)/commands)", [&control, allowed](const auto& req, auto& res) {
    if (!allowed(req, res)) return;
    try {
      auto j = nlohmann::json::parse(req.body);
      std::string device_id = req.matches[1];
      auto device = control.device(device_id);
      if (!device || !allowed_tenant(req, device->organization_id)) return json_response(res, {{"error", "not_found"}}, 404);
      auto cmd = control.create_command(device->organization_id, device_id, j.value("type", ""),
                                        j.value("now_epoch", 0L), j.value("ttl_seconds", 60L),
                                        j.value("max_retries", 3));
      json_response(res, {{"command", cmd}}, cmd.status == "failed" ? 400 : 200);
    } catch (...) {
      json_response(res, {{"error", "bad_request"}}, 400);
    }
  });
  server.Get("/fleet-control/v1/commands", [&control, allowed](const auto& req, auto& res) {
    if (!allowed(req, res)) return;
    json_response(res, {{"commands", control.commands(tenant_org(req), req.get_param_value("device_id"))}});
  });
  server.Post(R"(/fleet-control/v1/commands/([^/]+)/ack)", [&control, allowed](const auto& req, auto& res) {
    if (!allowed(req, res)) return;
    try {
      auto j = nlohmann::json::parse(req.body);
      auto ok = control.acknowledge_command(j.value("device_id", ""), req.matches[1], j.value("now_epoch", 0L));
      json_response(res, {{"ok", ok}}, ok ? 200 : 409);
    } catch (...) {
      json_response(res, {{"error", "bad_request"}}, 400);
    }
  });

  server.Get("/fleet-control/v1/alerts", [&control, allowed](const auto& req, auto& res) {
    if (!allowed(req, res)) return;
    json_response(res, {{"alerts", control.alerts(tenant_org(req), req.has_param("include_acknowledged"))}});
  });
  server.Post(R"(/fleet-control/v1/alerts/([^/]+)/ack)", [&control, allowed](const auto& req, auto& res) {
    if (!allowed(req, res)) return;
    auto ok = control.acknowledge_alert(tenant_org(req), req.matches[1], 0);
    json_response(res, {{"ok", ok}}, ok ? 200 : 404);
  });

  server.Get("/fleet-control/v1/metrics", [&control, allowed](const auto& req, auto& res) {
    if (!allowed(req, res)) return;
    json_response(res, control.metrics(tenant_org(req)));
  });
  server.Get(R"(/fleet-control/v1/dashboard/([^/]+)/([^/]+))", [&control, allowed](const auto& req, auto& res) {
    if (!allowed(req, res)) return;
    auto view = control.dashboard(req.matches[1], req.matches[2]);
    if (!tenant_org(req).empty() && req.matches[1] == "organization" && tenant_org(req) != req.matches[2]) {
      return json_response(res, {{"error", "tenant_forbidden"}}, 403);
    }
    json_response(res, view);
  });
}

FleetControlPlane& default_control_plane() {
  static FleetControlPlane* cp = [] {
    const char* env = std::getenv("REGISTER_MVP_FLEET_CONTROL_STORE");
    std::string path = env && *env ? std::string(env) : "data/fleet_control.json";
    return new FleetControlPlane(path);
  }();
  return *cp;
}

}  // namespace cloud::fleet_control
