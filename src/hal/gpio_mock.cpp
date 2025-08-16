#include "gpio.hpp"
#ifndef USE_LIBGPIOD
#include <iostream>
#include <memory>
#include <thread>
#include <vector>

namespace hal {

class MockLine : public Line {
public:
  MockLine(int id, bool out) : id_(id), out_(out) {}
  void write(bool v) override { if(out_) { val_=v; std::cerr<<"[MOCK GPIO] set "<<id_<<"="<<v<<"\n"; } }
  bool read() override { std::cerr<<"[MOCK GPIO] read "<<id_<<" -> "<<val_<<"\n"; return val_; }
private:
  int id_; bool out_; bool val_{false};
};

class MockChip : public Chip {
public:
  Line* request_line(int gpio, Direction dir, Pull, const std::string& name) override {
    (void)name;
    owned_.emplace_back(std::unique_ptr<MockLine>(new MockLine(gpio, dir==Direction::Out)));
    return owned_.back().get();
  }
private:
  std::vector<std::unique_ptr<MockLine>> owned_;
};

std::unique_ptr<Chip> make_chip(const std::string&) { return std::unique_ptr<Chip>(new MockChip()); }

void sleep_us(uint64_t us) { std::this_thread::sleep_for(std::chrono::microseconds(us)); }
void busy_wait_us(uint64_t us) { sleep_us(us); }

} // namespace hal
#endif

