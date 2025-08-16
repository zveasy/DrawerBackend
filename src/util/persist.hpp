#pragma once

#include <map>
#include <string>

namespace persist {

// Load key=value pairs from a simple text file. Returns true on success.
bool load_kv(const std::string& path, std::map<std::string, std::string>& out);

// Save key=value pairs to the given path. Returns true on success.
bool save_kv(const std::string& path, const std::map<std::string, std::string>& kv);

} // namespace persist

