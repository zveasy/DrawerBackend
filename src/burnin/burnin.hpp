#pragma once
#include <functional>
#include "config/config.hpp"

namespace burnin {
int run(const cfg::Config& cfg, std::function<bool(int)> fault_cb = nullptr, int cycles_override = -1);
}
