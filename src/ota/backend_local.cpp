#include "ota/backend.hpp"
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

namespace ota {

class LocalBackend : public IOtaBackend {
  std::string root_;
public:
  explicit LocalBackend(const std::string& root) : root_(root) {
    fs::create_directories(root_);
  }
  OtaResult install_bundle(const std::string& path) override {
    fs::create_directories(root_);
    fs::copy_file(path, fs::path(root_) / "installed", fs::copy_options::overwrite_existing);
    return {true, ""};
  }
  OtaResult mark_boot_ok() override {
    return {true, ""};
  }
  OtaResult mark_boot_fail() override {
    fs::remove(fs::path(root_) / "installed");
    return {true, ""};
  }
  BootEnv boot_env() override {
    BootEnv b; b.slot = "A"; b.boot_pending = fs::exists(fs::path(root_) / "installed"); return b; }
};

std::unique_ptr<IOtaBackend> make_local_backend(const std::string& root) {
  return std::make_unique<LocalBackend>(root);
}
} // namespace ota

