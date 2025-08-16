cmake_minimum_required(VERSION 3.16)
project(register_mvp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

option(USE_MOCK_GPIO "Use mock GPIO" ON)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  add_compile_options(-Wall -Wextra -Wpedantic -O2 -g)
endif()

# util
add_library(util util/log.cpp)
target_include_directories(util PUBLIC .)

# hal
add_library(hal hal/gpio.hpp hal/time.hpp)
target_include_directories(hal PUBLIC .)
target_link_libraries(hal PUBLIC util)

if(USE_MOCK_GPIO)
  target_sources(hal PRIVATE hal/gpio_mock.cpp)
  target_compile_definitions(hal PUBLIC USE_MOCK_GPIO=1)
else()
  include(cmake/gpiod.cmake)
  find_gpiod()
  target_sources(hal PRIVATE hal/gpio_gpiod.cpp)
  target_include_directories(hal PUBLIC ${GPIOD_INCLUDE_DIRS})
  target_link_libraries(hal PUBLIC ${GPIOD_LIBRARIES})
endif()

# sim
add_library(sim sim/sim.cpp)
target_include_directories(sim PUBLIC .)

# executable always uses mock chip
add_executable(register_mvp_sim main.cpp)
target_link_libraries(register_mvp_sim PRIVATE sim util hal)
target_include_directories(register_mvp_sim PRIVATE .)
if(NOT USE_MOCK_GPIO)
  target_sources(register_mvp_sim PRIVATE hal/gpio_mock.cpp)
endif()

enable_testing()
add_subdirectory(tests)
