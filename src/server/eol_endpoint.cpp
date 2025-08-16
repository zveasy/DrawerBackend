#include "server/eol_endpoint.hpp"
#include <thread>
#include <fstream>

namespace server {

EolEndpoint::EolEndpoint(eol::Runner& r, const cfg::Config& c) : runner_(r), cfg_(c) {}

void EolEndpoint::register_routes(httplib::Server& svr) {
  svr.Post("/eol/start", [this](const httplib::Request&, httplib::Response& res){
    if (busy_) { res.status = 409; res.set_content("{\"started\":false}", "application/json"); return; }
    busy_ = true;
    std::thread([this]{ last_ = runner_.run_once(cfg_); busy_ = false; }).detach();
    res.set_content("{\"started\":true}", "application/json");
  });

  svr.Get("/eol/status", [this](const httplib::Request&, httplib::Response& res){
    std::string step = busy_ && !runner_.status().empty() ? runner_.status().back().name : "idle";
    res.set_content("{\"busy\":" + std::string(busy_?"true":"false") + ",\"step\":\"" + step + "\"}", "application/json");
  });

  svr.Get("/eol/result", [this](const httplib::Request&, httplib::Response& res){
    if (last_.report_path.empty()) { res.status = 404; return; }
    std::ifstream in(last_.report_path);
    std::stringstream buf; buf << in.rdbuf();
    res.set_content(buf.str(), "application/json");
  });
}

} // namespace server
