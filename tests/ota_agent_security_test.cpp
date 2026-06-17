#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <iomanip>
#include <openssl/sha.h>
#include <optional>
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
  int installs{0};
  ota::OtaResult install_bundle(const std::string&) override { return {true, ""}; }
  ota::OtaResult mark_boot_ok() override { return {true, ""}; }
  ota::OtaResult mark_boot_fail() override { return {true, ""}; }
  ota::BootEnv boot_env() override { return {"A", false}; }
};

class StaticManifestFetcher : public ota::IManifestFetcher {
 public:
  explicit StaticManifestFetcher(std::string manifest) : manifest_(std::move(manifest)) {}

  ota::OtaResult fetch(const std::string&, std::string& manifest) override {
    manifest = manifest_;
    return {true, ""};
  }

 private:
  std::string manifest_;
};

cfg::Config base_config(const path& tmp, const std::string& channel = "stable",
                        const std::string& sha_override = "",
                        std::optional<std::string> extra_fields = std::nullopt,
                        bool include_sha = true) {
  path state = tmp / "state";
  create_directories(state);
  path artifact = tmp / "artifact.bin";
  std::string data = "artifact";
  std::ofstream(artifact) << data;
  std::string sha = sha_override.empty() ? hex_sha(data) : sha_override;
  path manifest = tmp / "release.json";
  std::ofstream(manifest) << "{\"channel\":\"" << channel
                          << "\",\"version\":\"1.1.0\",\"artifact_url\":\"file://"
                          << artifact.string() << "\""
                          << (include_sha ? ",\"sha256\":\"" + sha + "\"" : "")
                          << ",\"sig_ed25519\":\"bad\""
                          << (extra_fields ? "," + *extra_fields : "")
                          << "}";
  cfg::Config c = cfg::defaults();
  c.ota.enable = true;
  c.ota.feed_url = "file://" + manifest.string();
  c.ota.channel = "stable";
  c.ota.state_dir = state.string();
  c.ota.require_signed = true;
  c.aws.thing_name = "REG-01";
  return c;
}

void write_state_current_version(const cfg::Config& c, const std::string& version) {
  std::ofstream(c.ota.state_dir + "/state.json")
      << "current_version=" << version << "\n"
      << "pending_version=\n"
      << "boot_pending=0\n"
      << "last_bad=\n";
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

TEST(OtaAgentSecurity, RejectsMissingHashBeforeInstall) {
  path tmp = temp_directory_path() / "ota_missing_hash";
  remove_all(tmp);
  create_directories(tmp);
  NoopBackend backend;
  auto cfg = base_config(tmp, "stable", "", std::nullopt, false);
  cfg.ota.require_signed = false;
  ota::Agent agent(cfg, backend);
  auto res = agent.run_once();
  EXPECT_FALSE(res.ok);
  EXPECT_EQ("sha", res.reason);
}

TEST(OtaAgentSecurity, RejectsStaleVersionBeforeInstall) {
  path tmp = temp_directory_path() / "ota_stale_version";
  remove_all(tmp);
  create_directories(tmp);
  NoopBackend backend;
  auto cfg = base_config(tmp);
  cfg.ota.require_signed = false;
  write_state_current_version(cfg, "1.1.0");
  ota::Agent agent(cfg, backend);
  auto res = agent.run_once();
  EXPECT_FALSE(res.ok);
  EXPECT_EQ("version", res.reason);
}

TEST(OtaAgentSecurity, RejectsRevokedDeviceBeforeInstall) {
  path tmp = temp_directory_path() / "ota_revoked_device";
  remove_all(tmp);
  create_directories(tmp);
  NoopBackend backend;
  auto cfg = base_config(tmp, "stable", "", "\"revoked_devices\":[\"REG-01\"]");
  cfg.ota.require_signed = false;
  ota::Agent agent(cfg, backend);
  auto res = agent.run_once();
  EXPECT_FALSE(res.ok);
  EXPECT_EQ("revoked", res.reason);
}

TEST(OtaAgentSecurity, RejectsStagedRolloutExclusionBeforeInstall) {
  path tmp = temp_directory_path() / "ota_rollout_excluded";
  remove_all(tmp);
  create_directories(tmp);
  NoopBackend backend;
  auto cfg = base_config(tmp, "stable", "", "\"rollout_percent\":0");
  cfg.ota.require_signed = false;
  ota::Agent agent(cfg, backend);
  auto res = agent.run_once();
  EXPECT_FALSE(res.ok);
  EXPECT_EQ("rollout", res.reason);
}

TEST(OtaAgentSecurity, UsesManifestFetcherAbstractionForRemoteFeeds) {
  path tmp = temp_directory_path() / "ota_manifest_fetcher";
  remove_all(tmp);
  create_directories(tmp);
  path artifact = tmp / "artifact.bin";
  std::string data = "artifact";
  std::ofstream(artifact) << data;
  std::string manifest = "{\"channel\":\"pilot\",\"version\":\"1.1.0\",\"artifact_url\":\"file://" +
                         artifact.string() + "\",\"sha256\":\"" + hex_sha(data) + "\"}";
  StaticManifestFetcher fetcher(manifest);
  NoopBackend backend;
  auto cfg = base_config(tmp);
  cfg.ota.feed_url = "https://updates.example.invalid/releases/pilot.json";
  cfg.ota.channel = "pilot";
  cfg.ota.require_signed = false;
  ota::Agent agent(cfg, backend, fetcher);
  auto res = agent.run_once();
  EXPECT_TRUE(res.ok) << res.reason;
}
