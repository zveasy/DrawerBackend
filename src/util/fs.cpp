#include "util/fs.hpp"
#include <filesystem>

namespace fsutil {

bool ensure_dir(const std::string& path) {
  std::error_code ec;
  std::filesystem::create_directories(path, ec);
  return !ec;
}

} // namespace fsutil

