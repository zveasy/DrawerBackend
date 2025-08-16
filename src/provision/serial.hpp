#pragma once
#include <string>
#include <ctime>
#include "config/config.hpp"

namespace provision {
std::string make_serial(const cfg::Config& cfg, std::time_t now = std::time(nullptr));
bool write_serial_file(const std::string& path, const std::string& serial);
}
