#include "ota/backend.hpp"
#include <cstdlib>

namespace ota {

class RaucBackend : public IOtaBackend {
public:
  OtaResult install_bundle(const std::string& path) override {
    (void)path;
    return {false, "rauc not installed"};
  }
  OtaResult mark_boot_ok() override { return {true, ""}; }
  OtaResult mark_boot_fail() override { return {true, ""}; }
  BootEnv boot_env() override { return {"", false}; }
};

std::unique_ptr<IOtaBackend> make_rauc_backend() {
  return std::make_unique<RaucBackend>();
}

} // namespace ota

