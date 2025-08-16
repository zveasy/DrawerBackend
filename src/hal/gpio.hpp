#pragma once
#include <cstdint>
#include <chrono>
#include <string>
#include <memory>

// Simple GPIO HAL. When USE_LIBGPIOD is defined we use libgpiod v1.
// Otherwise, a mock prints actions (handy on laptops).

namespace hal {

enum class Direction { In, Out };
enum class Pull { None, Up, Down }; // pull ignored in this minimal v1 gpiod impl
enum class Edge { None, Rising, Falling, Both };

class Line {
public:
  virtual ~Line() = default;
  virtual void write(bool value) = 0;
  virtual bool read() = 0;
};

class Chip {
public:
  virtual ~Chip() = default;
  virtual Line* request_line(int gpio, Direction dir, Pull pull = Pull::None,
                             const std::string& name = "register_mvp") = 0;
};

std::unique_ptr<Chip> make_chip(const std::string& chip_name_or_path = "gpiochip0");

// Utility helpers
void sleep_us(uint64_t us);
void busy_wait_us(uint64_t us);

} // namespace hal

