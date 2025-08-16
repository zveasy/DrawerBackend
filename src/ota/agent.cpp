#include "ota/agent.hpp"
#include "util/ed25519_verify.hpp"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <map>
#include <openssl/sha.h>
#include "util/persist.hpp"

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
  std::map<std::string,std::string> kv;
  persist::load_kv(state_path(c), kv);
  if (kv.count("current_version")) s.current_version = kv["current_version"];
  if (kv.count("pending_version")) s.pending_version = kv["pending_version"];
  if (kv.count("boot_pending")) s.boot_pending = kv["boot_pending"] == "1";
  if (kv.count("last_bad")) {
    std::stringstream ss(kv["last_bad"]);
    std::string item; while (std::getline(ss,item,',')) if(!item.empty()) s.last_bad.push_back(item);
  }
  return true;
}

static bool save_state(const cfg::Config& c, const State& s) {
  std::map<std::string,std::string> kv;
  kv["current_version"] = s.current_version;
  kv["pending_version"] = s.pending_version;
  kv["boot_pending"] = s.boot_pending ? "1" : "0";
  std::string bad; for(size_t i=0;i<s.last_bad.size();++i){ if(i)bad+=","; bad+=s.last_bad[i]; }
  kv["last_bad"] = bad;
  fs::create_directories(c.ota.state_dir);
  return persist::save_kv(state_path(c), kv);
}

static std::string read_file(const std::string& p){ std::ifstream in(p, std::ios::binary); std::ostringstream ss; ss<<in.rdbuf(); return ss.str(); }

static std::string hex_sha256(const std::string& data) {
  unsigned char hash[SHA256_DIGEST_LENGTH];
  SHA256(reinterpret_cast<const unsigned char*>(data.data()), data.size(), hash);
  std::ostringstream oss; for(int i=0;i<SHA256_DIGEST_LENGTH;i++){ oss<<std::hex<<std::setw(2)<<std::setfill('0')<<(int)hash[i]; }
  return oss.str();
}

static std::string extract_string(const std::string& json, const std::string& key) {
  auto pos = json.find("\""+key+"\""); if(pos==std::string::npos) return ""; pos = json.find(':',pos); if(pos==std::string::npos) return ""; pos = json.find('"',pos); if(pos==std::string::npos) return ""; auto end = json.find('"',pos+1); if(end==std::string::npos) return ""; return json.substr(pos+1,end-pos-1);
}

static int extract_stage(const std::string& json, const std::string& key) {
  auto pos = json.find("\""+key+"\""); if(pos==std::string::npos) return 100; pos = json.find(':',pos); if(pos==std::string::npos) return 100; size_t end = json.find_first_not_of("0123456789", pos+1); return std::stoi(json.substr(pos+1,end-pos-1)); }

Agent::Agent(const cfg::Config& cfg, IOtaBackend& backend) : cfg_(cfg), backend_(backend) {}

int Agent::hash_device(const std::string& id) { return static_cast<int>(std::hash<std::string>{}(id)%100); }
bool Agent::allow(int h, int percent) { return h < percent; }

OtaResult Agent::run_once() {
  if (!cfg_.ota.enable) return {false, "disabled"};
  State st; load_state(cfg_, st);
  if (cfg_.ota.feed_url.rfind("file://",0)!=0) return {false, "bad feed"};
  std::string feed_path = cfg_.ota.feed_url.substr(7);
  std::string manifest = read_file(feed_path);
  std::string channel = extract_string(manifest, "channel");
  if (channel != cfg_.ota.channel) return {false,"channel"};
  std::string version = extract_string(manifest, "version");
  if (version <= st.current_version) return {false, "version"};
  std::string artifact = extract_string(manifest, "artifact_url");
  std::string sha = extract_string(manifest, "sha256");
  std::string sig = extract_string(manifest, "sig_ed25519");
  std::string verify_payload = manifest;
  auto spos = verify_payload.find("\"sig_ed25519\"");
  if (spos != std::string::npos) {
    auto epos = verify_payload.find(',', spos);
    if (epos == std::string::npos) epos = verify_payload.rfind('}');
    verify_payload.erase(spos, epos - spos);
  }
  if (cfg_.ota.require_signed && cfg_.ota.key_pub.size()>0) {
    std::string pub = read_file(cfg_.ota.key_pub);
    if (!ed25519::verify_pem(pub, verify_payload, sig)) return {false, "sig"};
  }
  std::string art_path = artifact.substr(7);
  std::string data = read_file(art_path);
  if (hex_sha256(data) != sha) return {false, "sha"};
  fs::create_directories(cfg_.ota.state_dir + "/downloads");
  std::string dest = cfg_.ota.state_dir + "/downloads/artifact";
  std::ofstream out(dest, std::ios::binary); out<<data; out.close();
  auto res = backend_.install_bundle(dest);
  if (!res.ok) return res;
  st.pending_version = version;
  st.boot_pending = true;
  save_state(cfg_, st);
  std::ofstream(cfg_.ota.state_dir + "/boot_pending").close();
  return {true, ""};
}

} // namespace ota

