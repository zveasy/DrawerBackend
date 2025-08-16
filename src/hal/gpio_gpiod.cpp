#include "gpio.hpp"
#ifdef USE_LIBGPIOD
#include <gpiod.h>
#include <stdexcept>
#include <iostream>
#include <thread>
#include <vector>

namespace hal {

class GpiodLine : public Line {
public:
  GpiodLine(gpiod_line* ln, bool is_out) : line_(ln), is_out_(is_out) {}
  ~GpiodLine() override {
    if (line_) gpiod_line_release(line_);
  }
  void write(bool v) override {
    if (!is_out_) throw std::runtime_error("write on input line");
    if (gpiod_line_set_value(line_, v ? 1 : 0) < 0) throw std::runtime_error("gpiod_line_set_value failed");
  }
  bool read() override {
    int r = gpiod_line_get_value(line_);
    if (r < 0) throw std::runtime_error("gpiod_line_get_value failed");
    return r == 1;
  }
private:
  gpiod_line* line_{};
  bool is_out_{};
};

class GpiodChip : public Chip {
public:
  explicit GpiodChip(const std::string& name) {
    chip_ = gpiod_chip_open_by_name(name.c_str());
    if (!chip_) {
      // also try path
      chip_ = gpiod_chip_open(name.c_str());
    }
    if (!chip_) throw std::runtime_error("Failed to open gpiochip: " + name);
  }
  ~GpiodChip() override {
    if (chip_) gpiod_chip_close(chip_);
  }
  Line* request_line(int gpio, Direction dir, Pull, const std::string& name) override {
    gpiod_line* ln = gpiod_chip_get_line(chip_, gpio);
    if (!ln) throw std::runtime_error("gpiod_chip_get_line failed");
    int rc;
    if (dir == Direction::Out)
      rc = gpiod_line_request_output(ln, name.c_str(), 0);
    else
      rc = gpiod_line_request_input(ln, name.c_str());
    if (rc < 0) throw std::runtime_error("gpiod_line_request_* failed");
    owned_.emplace_back(std::unique_ptr<GpiodLine>(new GpiodLine(ln, dir==Direction::Out)));
    return owned_.back().get();
  }
private:
  gpiod_chip* chip_{};
  std::vector<std::unique_ptr<GpiodLine>> owned_;
};

std::unique_ptr<Chip> make_chip(const std::string& name) { return std::unique_ptr<Chip>(new GpiodChip(name)); }

void sleep_us(uint64_t us) { std::this_thread::sleep_for(std::chrono::microseconds(us)); }
void busy_wait_us(uint64_t us) {
  auto start = std::chrono::steady_clock::now();
  while (std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now()-start).count() < (long long)us) {}
}

} // namespace hal
#else
// compiled out if not using libgpiod
#endif

