cmake_minimum_required(VERSION 3.16)
project(register_mvp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

option(USE_MOCK_GPIO "Build with a mock GPIO HAL (no libgpiod)" ON)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  add_compile_options(-Wall -Wextra -Wpedantic -O2 -g)
endif()

include(FetchContent)
FetchContent_Declare(cpphttplib
  GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
  GIT_TAG v0.15.3)
FetchContent_MakeAvailable(cpphttplib)
add_library(httplib::httplib ALIAS httplib)

find_package(nlohmann_json 3.2.0 REQUIRED)

# util
add_library(util
  util/log.cpp
  src/util/persist.cpp
  src/util/journal.cpp
  src/util/metrics.cpp
)
target_include_directories(util PUBLIC . src)

# hal
add_library(hal
  hal/gpio.hpp
  hal/time.hpp
)
target_include_directories(hal PUBLIC .)
target_link_libraries(hal PUBLIC util)
if(USE_MOCK_GPIO)
  target_sources(hal PRIVATE hal/gpio_mock.cpp)
  target_compile_definitions(hal PUBLIC USE_MOCK_GPIO=1)
else()
  include(cmake/gpiod.cmake)
  find_gpiod()
  target_sources(hal PRIVATE hal/gpio_gpiod.cpp)
  target_compile_definitions(hal PUBLIC USE_LIBGPIOD=1)
endif()

# drivers
add_library(drivers
  src/drivers/stepper.cpp
  src/drivers/hopper_parallel.cpp
  src/drivers/hx711.cpp
)
target_include_directories(drivers PUBLIC . src)
target_link_libraries(drivers PUBLIC hal)

# app
add_library(app
  src/app/shutter_fsm.cpp
  src/app/dispense_ctrl.cpp
  src/app/audit.cpp
  src/app/shutter_adapter.cpp
  src/app/dispense_adapter.cpp
  src/app/txn_engine.cpp
  src/app/selftest.cpp
)
target_include_directories(app PUBLIC . src)
target_link_libraries(app PUBLIC drivers hal util)

# server / api
add_library(server
  src/server/http_server.cpp
  src/ui/tui.cpp
)
target_include_directories(server PUBLIC . src)
target_link_libraries(server PUBLIC app httplib::httplib pthread)

# config
add_library(cfg
  src/config/config.cpp
)
target_include_directories(cfg PUBLIC . src)

# cloud
add_library(cloud
  src/cloud/iot_client.cpp
)
target_include_directories(cloud PUBLIC . src)
target_link_libraries(cloud PUBLIC util nlohmann_json::nlohmann_json paho-mqttpp3 paho-mqtt3as)

# main executable
add_executable(register_mvp src/main.cpp)
target_link_libraries(register_mvp PRIVATE app drivers hal util cfg server cloud)
target_include_directories(register_mvp PRIVATE . src)

enable_testing()
add_subdirectory(tests)

