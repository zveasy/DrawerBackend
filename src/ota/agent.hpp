#pragma once
#include "ota/backend.hpp"
#include "config/config.hpp"
#include <string>

namespace ota {

class IManifestFetcher {
public:
  virtual ~IManifestFetcher() = default;
  virtual OtaResult fetch(const std::string& url, std::string& manifest) = 0;
};

class Agent {
public:
  Agent(const cfg::Config& cfg, IOtaBackend& backend);
  Agent(const cfg::Config& cfg, IOtaBackend& backend, IManifestFetcher& fetcher);
  OtaResult run_once();

  static int hash_device(const std::string& id);
  static bool allow(int h, int percent);
private:
  cfg::Config cfg_;
  IOtaBackend& backend_;
  IManifestFetcher* fetcher_{nullptr};
};

} // namespace ota
