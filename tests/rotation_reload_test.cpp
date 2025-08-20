#include <gtest/gtest.h>
#include "cloud/tls_identity.hpp"
#include "config/config.hpp"
#include <filesystem>
#include <fstream>
#include <sys/stat.h>
#include <cstdlib>

TEST(Rotation, ReloadCert) {
  cfg::Config c = cfg::defaults();
  auto dir = std::filesystem::temp_directory_path() / "rotid";
  std::filesystem::create_directories(dir);
  std::string key = (dir / "device.key").string();
  std::string cert = (dir / "device.crt").string();
  {
    std::ofstream k(key); k << "k"; }
  chmod(key.c_str(), 0600);
  {
    std::ofstream ce(cert); ce << "cert1"; }
  setenv("AWS_KEY_ROT", key.c_str(), 1);
  setenv("AWS_CERT_ROT", cert.c_str(), 1);
  c.aws.key.env = "AWS_KEY_ROT"; c.aws.cert.env = "AWS_CERT_ROT"; c.identity.use_tpm = false;
  auto c1 = cloud::TlsIdentity::load_from_config(c);
  {
    std::ofstream ce(cert); ce << "cert2"; }
  auto c2 = cloud::TlsIdentity::load_from_config(c);
  EXPECT_NE(c1.cert_pem, c2.cert_pem);
}
