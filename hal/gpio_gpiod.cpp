#include "gpio.hpp"
#include <gpiod.h>
#include <stdexcept>
#include <vector>

namespace hal {

class GpiodLine : public Line {
public:
  GpiodLine(gpiod_line* ln, bool out) : line_(ln), out_(out) {}
  ~GpiodLine() override { if(line_) gpiod_line_release(line_); }
  void write(bool v) override {
    if(!out_) throw std::runtime_error("write on input line");
    if(gpiod_line_set_value(line_, v?1:0) < 0) throw std::runtime_error("gpiod_line_set_value");
  }
  bool read() override {
    int r = gpiod_line_get_value(line_);
    if(r < 0) throw std::runtime_error("gpiod_line_get_value");
    return r;
  }
private:
  gpiod_line* line_;
  bool out_;
};

class GpiodChip : public Chip {
public:
  explicit GpiodChip(const std::string& name) {
    chip_ = gpiod_chip_open_by_name(name.c_str());
    if(!chip_) chip_ = gpiod_chip_open(name.c_str());
    if(!chip_) throw std::runtime_error("open gpiochip");
  }
  ~GpiodChip() override { if(chip_) gpiod_chip_close(chip_); }
  Line* request_line(int gpio, Direction dir) override {
    gpiod_line* ln = gpiod_chip_get_line(chip_, gpio);
    if(!ln) throw std::runtime_error("gpiod_chip_get_line");
    int rc = (dir==Direction::Out)
      ? gpiod_line_request_output(ln, "register_mvp", 0)
      : gpiod_line_request_input(ln, "register_mvp");
    if(rc < 0) throw std::runtime_error("gpiod_line_request");
    owned_.emplace_back(new GpiodLine(ln, dir==Direction::Out));
    return owned_.back().get();
  }
private:
  gpiod_chip* chip_{};
  std::vector<std::unique_ptr<GpiodLine>> owned_;
};

std::unique_ptr<Chip> make_chip(const std::string& name) {
  return std::unique_ptr<Chip>(new GpiodChip(name));
}

} // namespace hal
