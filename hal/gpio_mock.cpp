#include "gpio.hpp"
#include "../util/log.hpp"
#include <map>
#include <memory>

namespace hal {

class MockLine : public Line {
public:
  explicit MockLine(int id) : id_(id) {}
  void write(bool v) override {
    val_ = v;
    LOG_DEBUG("write", {{"line", std::to_string(id_)}, {"val", v ? "1" : "0"}});
  }
  bool read() override {
    LOG_DEBUG("read", {{"line", std::to_string(id_)}, {"val", val_ ? "1" : "0"}});
    return val_;
  }
private:
  int id_;
  bool val_{false};
};

class MockChip : public Chip {
public:
  Line* request_line(int gpio, Direction) override {

    auto it = lines_.find(gpio);
    if (it == lines_.end()) {
      it = lines_.emplace(gpio, std::make_unique<MockLine>(gpio)).first;
    }
    return it->second.get();

    lines_[gpio] = std::make_unique<MockLine>(gpio);
    return lines_[gpio].get();

  }
private:
  std::map<int, std::unique_ptr<MockLine>> lines_;
};

std::unique_ptr<Chip> make_mock_chip() { return std::unique_ptr<Chip>(new MockChip()); }

#ifdef USE_MOCK_GPIO
std::unique_ptr<Chip> make_chip(const std::string&) { return make_mock_chip(); }
#endif

} // namespace hal
