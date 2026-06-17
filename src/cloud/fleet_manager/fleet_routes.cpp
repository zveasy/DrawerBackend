#include "cloud/fleet_manager/fleet_routes.hpp"

#include <nlohmann/json.hpp>

#include "util/log.hpp"

namespace cloud::fleet_manager {
namespace {

void json_response(httplib::Response& res, const nlohmann::json& body) {
  res.set_content(body.dump(), "application/json");
}

void not_found(httplib::Response& res, const std::string& drawer_id) {
  res.status = 404;
  json_response(res, {{"error", "device_not_found"}, {"drawer_id", drawer_id}});
}

}  // namespace

void register_fleet_routes(httplib::Server& server, FleetManager& manager) {
  server.Get("/fleet/devices", [&manager](const httplib::Request&, httplib::Response& res) {
    nlohmann::json devices = nlohmann::json::array();
    for (const auto& twin : manager.list()) {
      devices.push_back({{"drawer_id", twin.drawer_id},
                         {"merchant_id", twin.merchant_id},
                         {"firmware_version", twin.firmware.current_version},
                         {"hardware_revision", twin.firmware.hardware_revision},
                         {"health_score", twin.health.score},
                         {"connectivity_status", twin.connectivity_status},
                         {"last_telemetry_at", twin.last_telemetry_at},
                         {"active_alerts", twin.alerts.size()}});
    }
    json_response(res, {{"devices", devices}});
  });

  server.Get(R"(/fleet/device/([^/]+))",
             [&manager](const httplib::Request& req, httplib::Response& res) {
               auto twin = manager.get(req.matches[1]);
               if (!twin) return not_found(res, req.matches[1]);
               json_response(res, *twin);
             });

  server.Get(R"(/fleet/device/([^/]+)/health)",
             [&manager](const httplib::Request& req, httplib::Response& res) {
               auto twin = manager.get(req.matches[1]);
               if (!twin) return not_found(res, req.matches[1]);
               json_response(res, {{"drawer_id", twin->drawer_id},
                                   {"health", twin->health},
                                   {"maintenance", twin->maintenance},
                                   {"alerts", twin->alerts}});
             });

  server.Get(R"(/fleet/device/([^/]+)/history)",
             [&manager](const httplib::Request& req, httplib::Response& res) {
               auto twin = manager.get(req.matches[1]);
               if (!twin) return not_found(res, req.matches[1]);
               json_response(res, {{"drawer_id", twin->drawer_id},
                                   {"fault_history", twin->fault_history},
                                   {"maintenance_history", twin->maintenance_history},
                                   {"timeline", twin->history}});
             });

  server.Get(R"(/fleet/device/([^/]+)/inventory)",
             [&manager](const httplib::Request& req, httplib::Response& res) {
               auto twin = manager.get(req.matches[1]);
               if (!twin) return not_found(res, req.matches[1]);
               json_response(res, {{"drawer_id", twin->drawer_id},
                                   {"inventory", twin->inventory},
                                   {"forecast", twin->inventory_forecast}});
             });

  server.Get("/fleet/metrics", [&manager](const httplib::Request&, httplib::Response& res) {
    json_response(res, manager.metrics());
  });
}

}  // namespace cloud::fleet_manager
