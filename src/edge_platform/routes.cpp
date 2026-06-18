#include "edge_platform/routes.hpp"

#include <algorithm>

#include <nlohmann/json.hpp>

#include "util/log.hpp"

namespace edge_platform {
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

bool tenant_allowed(const httplib::Request& req, const std::string& tenant) {
  auto scoped = tenant_id(req);
  return scoped.empty() || tenant.empty() || scoped == tenant;
}

bool device_allowed(EdgePlatformService& service, const httplib::Request& req, const std::string& device_id) {
  auto tenant = tenant_id(req);
  if (tenant.empty()) return true;
  auto devices = service.devices(tenant);
  return std::find_if(devices.begin(), devices.end(), [&](const auto& d) {
    return d.device_id == device_id;
  }) != devices.end();
}

}  // namespace

void register_edge_routes(httplib::Server& server, EdgePlatformService& service,
                          AccessCheck access_check) {
  auto allowed = [access_check](const httplib::Request& req, httplib::Response& res) {
    if (access_check && !access_check(req, res)) return false;
    util::log(util::LogLevel::Info, "edge_api_access", {{"src", req.remote_addr}, {"route", req.path}});
    return true;
  };

  server.Get("/edge/v1/devices", [&service, allowed](const auto& req, auto& res) {
    if (!allowed(req, res)) return;
    json_response(res, {{"devices", service.devices(tenant_id(req))}});
  });

  server.Post("/edge/v1/devices", [&service, allowed](const auto& req, auto& res) {
    if (!allowed(req, res)) return;
    try {
      auto device = nlohmann::json::parse(req.body).template get<EdgeDevice>();
      if (!tenant_allowed(req, device.tenant_id)) return json_response(res, {{"error", "tenant_forbidden"}}, 403);
      auto ok = service.register_device(device);
      json_response(res, {{"ok", ok}, {"device", device}}, ok ? 200 : 400);
    } catch (...) {
      json_response(res, {{"error", "bad_request"}}, 400);
    }
  });

  server.Get(R"(/edge/v1/devices/([^/]+)/capabilities)", [&service, allowed](const auto& req, auto& res) {
    if (!allowed(req, res)) return;
    if (!device_allowed(service, req, req.matches[1])) return json_response(res, {{"error", "tenant_forbidden"}}, 403);
    json_response(res, {{"capabilities", service.capabilities(req.matches[1])}});
  });

  server.Post(R"(/edge/v1/devices/([^/]+)/capabilities)", [&service, allowed](const auto& req, auto& res) {
    if (!allowed(req, res)) return;
    if (!device_allowed(service, req, req.matches[1])) return json_response(res, {{"error", "tenant_forbidden"}}, 403);
    try {
      auto cap = nlohmann::json::parse(req.body).template get<CapabilityMetadata>();
      auto ok = service.register_capability(req.matches[1], cap);
      json_response(res, {{"ok", ok}, {"capability", cap}}, ok ? 200 : 400);
    } catch (...) {
      json_response(res, {{"error", "bad_request"}}, 400);
    }
  });

  server.Get("/edge/v1/drivers", [&service, allowed](const auto& req, auto& res) {
    if (!allowed(req, res)) return;
    json_response(res, {{"drivers", service.drivers()}});
  });

  server.Post(R"(/edge/v1/devices/([^/]+)/commands)", [&service, allowed](const auto& req, auto& res) {
    if (!allowed(req, res)) return;
    if (!device_allowed(service, req, req.matches[1])) return json_response(res, {{"error", "tenant_forbidden"}}, 403);
    try {
      auto command = nlohmann::json::parse(req.body).template get<EdgeCommandRequest>();
      command.device_id = req.matches[1];
      auto result = service.execute_command(command);
      json_response(res, {{"result", result}}, result.executed ? 200 : 409);
    } catch (...) {
      json_response(res, {{"error", "bad_request"}}, 400);
    }
  });

  server.Get(R"(/edge/v1/devices/([^/]+)/inventory)", [&service, allowed](const auto& req, auto& res) {
    if (!allowed(req, res)) return;
    if (!device_allowed(service, req, req.matches[1])) return json_response(res, {{"error", "tenant_forbidden"}}, 403);
    json_response(res, service.inventory(req.matches[1]));
  });

  server.Post(R"(/edge/v1/devices/([^/]+)/inventory)", [&service, allowed](const auto& req, auto& res) {
    if (!allowed(req, res)) return;
    if (!device_allowed(service, req, req.matches[1])) return json_response(res, {{"error", "tenant_forbidden"}}, 403);
    try {
      auto inv = nlohmann::json::parse(req.body).template get<EdgeInventoryView>();
      inv.device_id = req.matches[1];
      auto ok = service.update_inventory(inv);
      json_response(res, {{"ok", ok}, {"inventory", inv}}, ok ? 200 : 400);
    } catch (...) {
      json_response(res, {{"error", "bad_request"}}, 400);
    }
  });

  server.Post(R"(/edge/v1/devices/([^/]+)/simulate)", [&service, allowed](const auto& req, auto& res) {
    if (!allowed(req, res)) return;
    if (!device_allowed(service, req, req.matches[1])) return json_response(res, {{"error", "tenant_forbidden"}}, 403);
    try {
      auto j = nlohmann::json::parse(req.body);
      auto state = service.simulate(req.matches[1], j.value("action", ""),
                                    j.value("parameters", nlohmann::json::object()),
                                    j.value("now_epoch", 0L));
      json_response(res, state);
    } catch (...) {
      json_response(res, {{"error", "bad_request"}}, 400);
    }
  });

  server.Get(R"(/edge/v1/devices/([^/]+)/health)", [&service, allowed](const auto& req, auto& res) {
    if (!allowed(req, res)) return;
    if (!device_allowed(service, req, req.matches[1])) return json_response(res, {{"error", "tenant_forbidden"}}, 403);
    json_response(res, service.health(req.matches[1]));
  });

  server.Get("/edge/v1/events", [&service, allowed](const auto& req, auto& res) {
    if (!allowed(req, res)) return;
    json_response(res, {{"events", service.events(tenant_id(req))}});
  });
}

}  // namespace edge_platform
