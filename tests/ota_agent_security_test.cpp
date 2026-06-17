#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <iomanip>
#include <openssl/sha.h>
#include <sstream>

#include "ota/agent.hpp"

using namespace std::filesystem;

namespace {

std::string hex_sha(const std::string& d) {
  unsigned char h[SHA256_DIGEST_LENGTH];
  SHA256(reinterpret_cast<const unsigned char*>(d.data()), d.size(), h);
  std::ostringstream o;
  o << std::hex << std::setfill('0');
  for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) o << std::setw(2) << static_cast<int>(h[i]);
  return o.str();
}

class NoopBackend : public ota::IOtaBackend {
 public:
  ota::OtaResult install_bundle(const std::string&) override { return {true, ""}; }
  ota::OtaResult mark_boot_ok() override { return {true, ""}; }
  ota::OtaResult mark_boot_fail() override { return {true, ""}; }
  ota::BootEnv boot_env() override { return {"A", false}; }
};

cfg::Config base_config(const path& tmp, const std::string& channel = "stable",
                        const std::string& sha_override = "") {
  path state = tmp / "state";
  create_directories(state);
  path artifact = tmp / "artifact.bin";
  std::string data = "artifact";
  std::ofstream(artifact) << data;
  std::string sha = sha_override.empty() ? hex_sha(data) : sha_override;
  path manifest = tmp / "release.json";
  std::ofstream(manifest) << "{\"channel\":\"" << channel
                          << "\",\"version\":\"1.1.0\",\"artifact_url\":\"file://"
                          << artifact.string() << "\",\"sha256\":\"" << sha
                          << "\",\"sig_ed25519\":\"bad\"}";
  cfg::Config c = cfg::defaults();
  c.ota.enable = true;
  c.ota.feed_url = "file://" + manifest.string();
  c.ota.channel = "stable";
  c.ota.state_dir = state.string();
  c.ota.require_signed = true;
  return c;
}

}  // namespace

TEST(OtaAgentSecurity, RejectsSignedModeWithoutVerificationKey) {
  path tmp = temp_directory_path() / "ota_missing_key";
  remove_all(tmp);
  create_directories(tmp);
  NoopBackend backend;
  auto cfg = base_config(tmp);
  ota::Agent agent(cfg, backend);
  auto res = agent.run_once();
  EXPECT_FALSE(res.ok);
  EXPECT_EQ("sig_key", res.reason);
}

TEST(OtaAgentSecurity, RejectsBadChannelBeforeInstall) {
  path tmp = temp_directory_path() / "ota_bad_channel";
  remove_all(tmp);
  create_directories(tmp);
  NoopBackend backend;
  auto cfg = base_config(tmp, "beta");
  cfg.ota.require_signed = false;
  ota::Agent agent(cfg, backend);
  auto res = agent.run_once();
  EXPECT_FALSE(res.ok);
  EXPECT_EQ("channel", res.reason);
}

TEST(OtaAgentSecurity, RejectsBadShaBeforeInstall) {
  path tmp = temp_directory_path() / "ota_bad_sha";
  remove_all(tmp);
  create_directories(tmp);
  NoopBackend backend;
  auto cfg = base_config(tmp, "stable", "00");
  cfg.ota.require_signed = false;
  ota::Agent agent(cfg, backend);
  auto res = agent.run_once();
  EXPECT_FALSE(res.ok);
  EXPECT_EQ("sha", res.reason);
}
