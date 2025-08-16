#pragma once
#include <string>
#include <memory>

namespace ota {

struct OtaResult { bool ok; std::string reason; };
struct BootEnv { std::string slot; bool boot_pending{false}; };

class IOtaBackend {
public:
  virtual ~IOtaBackend() = default;
  virtual OtaResult install_bundle(const std::string& path) = 0;
  virtual OtaResult mark_boot_ok() = 0;
  virtual OtaResult mark_boot_fail() = 0;
  virtual BootEnv boot_env() = 0;
};

std::unique_ptr<IOtaBackend> make_local_backend(const std::string& root);
std::unique_ptr<IOtaBackend> make_rauc_backend();

} // namespace ota

