#include "server/version_endpoint.hpp"
#include "util/version.hpp"
#include <sstream>

namespace server {

void register_version_routes(httplib::Server& svr) {
  svr.Get("/version", [](const httplib::Request&, httplib::Response& res) {
    std::ostringstream oss;
    oss << "{\"version\":\"" << appver::version() << "\","
        << "\"git\":\"" << appver::git() << "\","
        << "\"build_time\":\"" << appver::build_time() << "\","
        << "\"schema\":1}";
    res.set_content(oss.str(), "application/json");
  });

  svr.Get("/healthz", [](const httplib::Request&, httplib::Response& res) {
    res.set_content("{\"ok\":true}", "application/json");
  });
}

} // namespace server

