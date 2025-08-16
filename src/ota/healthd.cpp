#include "ota/healthd.hpp"
#include "ota/agent.hpp"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <map>
#include "util/persist.hpp"
#include <httplib.h>

namespace fs = std::filesystem;

namespace ota {

struct State {
  std::string current_version{"0.0.0"};
  std::string pending_version;
  bool boot_pending{false};
  std::vector<std::string> last_bad;
};

static std::string state_path(const cfg::Config& c) { return c.ota.state_dir + "/state.json"; }

static bool load_state(const cfg::Config& c, State& s) {
  std::map<std::string,std::string> kv; persist::load_kv(state_path(c), kv);
  if (kv.count("current_version")) s.current_version = kv["current_version"];
  if (kv.count("pending_version")) s.pending_version = kv["pending_version"];
  if (kv.count("boot_pending")) s.boot_pending = kv["boot_pending"] == "1";
  if (kv.count("last_bad")) {
    std::stringstream ss(kv["last_bad"]); std::string item; while(std::getline(ss,item,',')) if(!item.empty()) s.last_bad.push_back(item);
  }
  return true;
}

static bool save_state(const cfg::Config& c, const State& s) {
  std::map<std::string,std::string> kv;
  kv["current_version"] = s.current_version;
  kv["pending_version"] = s.pending_version;
  kv["boot_pending"] = s.boot_pending?"1":"0";
  std::string bad; for(size_t i=0;i<s.last_bad.size();++i){ if(i)bad+=","; bad+=s.last_bad[i]; }
  kv["last_bad"] = bad;
  fs::create_directories(c.ota.state_dir);
  return persist::save_kv(state_path(c), kv);
}

Healthd::Healthd(const cfg::Config& cfg, IOtaBackend& backend) : cfg_(cfg), backend_(backend) {}

OtaResult Healthd::run_once(const std::string& base_url) {
  State st; load_state(cfg_, st);
  if (!st.boot_pending) return {true, "no pending"};
  std::string host = base_url; int port = 80;
  auto pos = host.find("://"); if (pos != std::string::npos) host = host.substr(pos+3);
  pos = host.find(':'); if (pos != std::string::npos) { port = std::stoi(host.substr(pos+1)); host = host.substr(0,pos); }
  httplib::Client cli(host.c_str(), port);
  cli.set_read_timeout(cfg_.ota.health_grace_seconds,0);
  auto res = cli.Get("/healthz");
  bool ok = res && res->status == 200 && res->body.find("\"ok\":true") != std::string::npos;
  if (ok) {
    backend_.mark_boot_ok();
    st.current_version = st.pending_version;
    st.pending_version.clear();
    st.boot_pending = false;
    save_state(cfg_, st);
    fs::remove(cfg_.ota.state_dir + "/boot_pending");
    return {true, ""};
  } else {
    backend_.mark_boot_fail();
    if(!st.pending_version.empty()) st.last_bad.push_back(st.pending_version);
    st.pending_version.clear();
    st.boot_pending = false;
    save_state(cfg_, st);
    fs::remove(cfg_.ota.state_dir + "/boot_pending");
    return {false, "fail"};
  }
}

} // namespace ota

