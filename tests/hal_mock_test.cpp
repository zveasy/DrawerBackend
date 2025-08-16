#include <gtest/gtest.h>
#include "../hal/gpio.hpp"

TEST(HalMock, WriteRead) {
  auto chip = hal::make_mock_chip();
  auto line = chip->request_line(1, hal::Direction::Out);
  line->write(true);
  EXPECT_TRUE(line->read());
}
