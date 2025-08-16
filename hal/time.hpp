#pragma once
#include <cstdint>
#include <chrono>
#include <thread>

namespace hal {
inline void sleep_us(uint64_t us) {
  std::this_thread::sleep_for(std::chrono::microseconds(us));
}
inline uint64_t now_ns() {
  return std::chrono::duration_cast<std::chrono::nanoseconds>(
           std::chrono::steady_clock::now().time_since_epoch()).count();
}
} // namespace hal
