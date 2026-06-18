#include "cloud/fleet_operations/routes.hpp"

#include <chrono>

#include <nlohmann/json.hpp>

#include "util/log.hpp"

namespace cloud::fleet_operations {
namespace {

void json_response(httplib::Response& res, const nlohmann::json& body, int status = 200) {
  res.status = status;
  res.set_content(body.dump(), "application/json");
}

long now_epoch() {
  return std::chrono::duration_cast<std::chrono::seconds>(
             std::chrono::system_clock::now().time_since_epoch())
      .count();
}

std::string tenant_id(const httplib::Request& req) {
  return req.get_header_value("X-Org-Id");
}

bool require_tenant(const httplib::Request& req, httplib::Response& res) {
  if (!tenant_id(req).empty()) return true;
  json_response(res, {{"error", "tenant_required"}}, 400);
  return false;
}

long request_time(const nlohmann::json& body) {
  return body.value("now_epoch", now_epoch());
}

}  // namespace

void register_fleet_operations_routes(httplib::Server& server,
                                      FleetOperationsService& service,
                                      AccessCheck access_check) {
  auto allowed = [access_check](const httplib::Request& req, httplib::Response& res) {
    if (access_check && !access_check(req, res)) return false;
    if (!require_tenant(req, res)) return false;
    util::log(util::LogLevel::Info, "fleet_operations_api_access",
              {{"src", req.remote_addr}, {"route", req.path},
               {"tenant_id", tenant_id(req)}});
    return true;
  };

  server.Post("/fleet/v1/devices/enroll", [&service, allowed](const auto& req, auto& res) {
    if (!allowed(req, res)) return;
    try {
      auto body = nlohmann::json::parse(req.body);
      auto device = body.template get<FleetDevice>();
      if (device.tenant_id != tenant_id(req))
        return json_response(res, {{"error", "tenant_forbidden"}}, 403);
      std::string error;
      auto ok = service.enroll(device, body.value("actor_id", ""), request_time(body), &error);
      json_response(res, {{"ok", ok}, {"error", error}, {"device", device}}, ok ? 201 : 409);
    } catch (...) {
      json_response(res, {{"error", "bad_request"}}, 400);
    }
  });

  server.Get("/fleet/v1/devices", [&service, allowed](const auto& req, auto& res) {
    if (!allowed(req, res)) return;
    json_response(res, {{"devices", service.devices(tenant_id(req))}});
  });

  server.Get(R"(/fleet/v1/devices/([^/]+))", [&service, allowed](const auto& req, auto& res) {
    if (!allowed(req, res)) return;
    auto device = service.device(tenant_id(req), req.matches[1]);
    if (!device) return json_response(res, {{"error", "not_found"}}, 404);
    json_response(res, *device);
  });

  server.Patch(R"(/fleet/v1/devices/([^/]+))", [&service, allowed](const auto& req, auto& res) {
    if (!allowed(req, res)) return;
    try {
      auto body = nlohmann::json::parse(req.body);
      std::string error;
      auto ok = service.update_device(tenant_id(req), req.matches[1], body,
                                      request_time(body), &error);
      json_response(res, {{"ok", ok}, {"error", error}}, ok ? 200 : 409);
    } catch (...) {
      json_response(res, {{"error", "bad_request"}}, 400);
    }
  });

  server.Post(R"(/fleet/v1/devices/([^/]+)/retire)", [&service, allowed](const auto& req, auto& res) {
    if (!allowed(req, res)) return;
    try {
      auto body = req.body.empty() ? nlohmann::json::object() : nlohmann::json::parse(req.body);
      std::string error;
      auto ok = service.retire_device(tenant_id(req), req.matches[1], request_time(body), &error);
      json_response(res, {{"ok", ok}, {"error", error}}, ok ? 200 : 409);
    } catch (...) {
      json_response(res, {{"error", "bad_request"}}, 400);
    }
  });

  server.Post("/fleet/v1/heartbeats", [&service, allowed](const auto& req, auto& res) {
    if (!allowed(req, res)) return;
    try {
      auto body = nlohmann::json::parse(req.body);
      auto heartbeat = body.template get<FleetHeartbeat>();
      if (heartbeat.tenant_id != tenant_id(req))
        return json_response(res, {{"error", "tenant_forbidden"}}, 403);
      std::string error;
      auto ok = service.receive_heartbeat(heartbeat, body.value("received_at", now_epoch()), &error);
      json_response(res, {{"ok", ok}, {"error", error}}, ok ? 202 : 409);
    } catch (...) {
      json_response(res, {{"error", "bad_request"}}, 400);
    }
  });

  server.Get(R"(/fleet/v1/devices/([^/]+)/health)", [&service, allowed](const auto& req, auto& res) {
    if (!allowed(req, res)) return;
    auto body = service.health(tenant_id(req), req.matches[1],
                               req.has_param("now_epoch") ? std::stol(req.get_param_value("now_epoch"))
                                                          : now_epoch());
    json_response(res, body, body.value("found", false) ? 200 : 404);
  });

  server.Get(R"(/fleet/v1/devices/([^/]+)/freshness)", [&service, allowed](const auto& req, auto& res) {
    if (!allowed(req, res)) return;
    auto body = service.freshness(tenant_id(req), req.matches[1],
                                  req.has_param("now_epoch") ? std::stol(req.get_param_value("now_epoch"))
                                                             : now_epoch());
    json_response(res, body, body.value("found", false) ? 200 : 404);
  });

  server.Post("/fleet/v1/commands", [&service, allowed](const auto& req, auto& res) {
    if (!allowed(req, res)) return;
    try {
      auto body = nlohmann::json::parse(req.body);
      auto command = body.template get<FleetCommand>();
      if (command.tenant_id != tenant_id(req))
        return json_response(res, {{"error", "tenant_forbidden"}}, 403);
      auto result = service.create_command(command, request_time(body));
      int status = result.state == "dispatched" ? 201 :
                   result.state == "policy_checking" ? 202 : 409;
      json_response(res, {{"command", result}}, status);
    } catch (...) {
      json_response(res, {{"error", "bad_request"}}, 400);
    }
  });

  server.Get("/fleet/v1/commands", [&service, allowed](const auto& req, auto& res) {
    if (!allowed(req, res)) return;
    service.expire_commands(now_epoch());
    json_response(res, {{"commands", service.commands(tenant_id(req))}});
  });

  server.Get(R"(/fleet/v1/commands/([^/]+))", [&service, allowed](const auto& req, auto& res) {
    if (!allowed(req, res)) return;
    service.expire_commands(now_epoch());
    auto command = service.command(tenant_id(req), req.matches[1]);
    if (!command) return json_response(res, {{"error", "not_found"}}, 404);
    json_response(res, *command);
  });

  auto command_action = [&service, allowed](const std::string& suffix, auto action) {
    (void)suffix;
    return [&service, allowed, action](const auto& req, auto& res) {
      if (!allowed(req, res)) return;
      try {
        auto body = req.body.empty() ? nlohmann::json::object() : nlohmann::json::parse(req.body);
        std::string error;
        auto ok = action(service, tenant_id(req), std::string(req.matches[1]), body, &error);
        json_response(res, {{"ok", ok}, {"error", error}}, ok ? 200 : 409);
      } catch (...) {
        json_response(res, {{"error", "bad_request"}}, 400);
      }
    };
  };

  server.Post(R"(/fleet/v1/commands/([^/]+)/cancel)",
              command_action("cancel", [](auto& svc, const auto& tenant, const auto& id,
                                           const auto& body, auto* error) {
                return svc.cancel_command(tenant, id, request_time(body), error);
              }));
  server.Post(R"(/fleet/v1/commands/([^/]+)/ack)",
              command_action("ack", [](auto& svc, const auto& tenant, const auto& id,
                                        const auto& body, auto* error) {
                return svc.acknowledge_command(tenant, id, request_time(body), error);
              }));
  server.Post(R"(/fleet/v1/commands/([^/]+)/complete)",
              command_action("complete", [](auto& svc, const auto& tenant, const auto& id,
                                             const auto& body, auto* error) {
                return svc.complete_command(tenant, id, body.value("success", false),
                                            body.value("result", ""), request_time(body), error);
              }));

  server.Get("/fleet/v1/risk", [&service, allowed](const auto& req, auto& res) {
    if (!allowed(req, res)) return;
    long now = req.has_param("now_epoch") ? std::stol(req.get_param_value("now_epoch")) : now_epoch();
    json_response(res, {{"risk", service.fleet_risk(tenant_id(req), now)}});
  });
  server.Get(R"(/fleet/v1/devices/([^/]+)/risk)", [&service, allowed](const auto& req, auto& res) {
    if (!allowed(req, res)) return;
    long now = req.has_param("now_epoch") ? std::stol(req.get_param_value("now_epoch")) : now_epoch();
    if (!service.device(tenant_id(req), req.matches[1]))
      return json_response(res, {{"error", "not_found"}}, 404);
    json_response(res, service.device_risk(tenant_id(req), req.matches[1], now));
  });
  server.Get(R"(/fleet/v1/locations/([^/]+)/risk)", [&service, allowed](const auto& req, auto& res) {
    if (!allowed(req, res)) return;
    long now = req.has_param("now_epoch") ? std::stol(req.get_param_value("now_epoch")) : now_epoch();
    json_response(res, service.location_risk(tenant_id(req), req.matches[1], now));
  });

  server.Post(R"(/fleet/v1/devices/([^/]+)/quarantine)", [&service, allowed](const auto& req, auto& res) {
    if (!allowed(req, res)) return;
    try {
      auto body = req.body.empty() ? nlohmann::json::object() : nlohmann::json::parse(req.body);
      std::string error;
      auto ok = service.quarantine(tenant_id(req), req.matches[1], body.value("reason", ""),
                                   request_time(body), &error);
      json_response(res, {{"ok", ok}, {"error", error}}, ok ? 200 : 409);
    } catch (...) {
      json_response(res, {{"error", "bad_request"}}, 400);
    }
  });
  server.Post(R"(/fleet/v1/devices/([^/]+)/recover)", [&service, allowed](const auto& req, auto& res) {
    if (!allowed(req, res)) return;
    try {
      auto body = req.body.empty() ? nlohmann::json::object() : nlohmann::json::parse(req.body);
      std::string error;
      auto ok = service.recover(tenant_id(req), req.matches[1], request_time(body), &error);
      json_response(res, {{"ok", ok}, {"error", error}}, ok ? 200 : 409);
    } catch (...) {
      json_response(res, {{"error", "bad_request"}}, 400);
    }
  });
  server.Get("/fleet/v1/quarantine", [&service, allowed](const auto& req, auto& res) {
    if (!allowed(req, res)) return;
    json_response(res, {{"devices", service.quarantined(tenant_id(req))}});
  });
}

}  // namespace cloud::fleet_operations
