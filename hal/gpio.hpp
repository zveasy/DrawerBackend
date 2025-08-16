#pragma once
#include <memory>
#include <string>

namespace hal {

enum class Direction { In, Out };

class Line {
public:
  virtual ~Line() = default;
  virtual void write(bool value) = 0;
  virtual bool read() = 0;
};

class Chip {
public:
  virtual ~Chip() = default;
  virtual Line* request_line(int gpio, Direction dir) = 0;
};

std::unique_ptr<Chip> make_chip(const std::string& name = "gpiochip0");
std::unique_ptr<Chip> make_mock_chip();

#ifndef USE_LIBGPIOD
namespace mock {
// Helpers for tests: manually drive inputs and inspect outputs on the mock
// backend. These have no effect when using the real libgpiod backend.
void set_input(int gpio, bool value);
bool get_output(int gpio);
} // namespace mock
#endif

} // namespace hal
