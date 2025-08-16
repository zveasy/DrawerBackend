#include "gpio.hpp"
#include "../util/log.hpp"
#include <map>
#include <memory>

namespace hal {

namespace {
// Storage for all mock GPIO line states. Both outputs written by the code
// under test and inputs driven by tests live here.
std::map<int, bool> g_values;
}

class MockLine : public Line {
public:
  explicit MockLine(int id) : id_(id) {}
  void write(bool v) override {
    g_values[id_] = v;
    LOG_DEBUG("write", {{"line", std::to_string(id_)}, {"val", v ? "1" : "0"}});
  }
  bool read() override {
    bool v = g_values[id_];
    LOG_DEBUG("read", {{"line", std::to_string(id_)}, {"val", v ? "1" : "0"}});
    return v;
  }
private:
  int id_;
};

class MockChip : public Chip {
public:
  Line* request_line(int gpio, Direction) override {
    auto it = lines_.find(gpio);
    if (it == lines_.end()) {
      it = lines_.emplace(gpio, std::make_unique<MockLine>(gpio)).first;
      g_values[gpio] = false;
    }
    return it->second.get();
  }
private:
  std::map<int, std::unique_ptr<MockLine>> lines_;
};

std::unique_ptr<Chip> make_mock_chip() {
  return std::unique_ptr<Chip>(new MockChip());
}

#ifdef USE_MOCK_GPIO
std::unique_ptr<Chip> make_chip(const std::string&) { return make_mock_chip(); }
#endif

namespace mock {
void set_input(int gpio, bool value) { g_values[gpio] = value; }
bool get_output(int gpio) { return g_values[gpio]; }
} // namespace mock

} // namespace hal

