cmake_minimum_required(VERSION 3.16)
project(register_mvp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

option(USE_MOCK_GPIO "Build with a mock GPIO HAL (no libgpiod)" ON)
option(USE_AWS_SDK "Build with AWS IoT CRT MQTT client" OFF)
option(USE_NLOHMANN_JSON "Use nlohmann/json single header" ON)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  add_compile_options(-Wall -Wextra -Wpedantic -O2 -g)
endif()

include(FetchContent)
FetchContent_Declare(cpphttplib
  GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
  GIT_TAG v0.15.3)
FetchContent_MakeAvailable(cpphttplib)
add_library(httplib::httplib ALIAS httplib)

if(USE_NLOHMANN_JSON)
  FetchContent_Declare(nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.2)
  FetchContent_MakeAvailable(nlohmann_json)
endif()

# util
add_library(util
  util/log.cpp
  src/util/persist.cpp
  src/util/journal.cpp
  src/util/metrics.cpp
  src/util/fs.cpp
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
  src/cloud/disk_queue.cpp
  src/cloud/shadow.cpp
)
target_include_directories(cloud PUBLIC . src)
target_link_libraries(cloud PUBLIC util)
if(USE_NLOHMANN_JSON)
  target_link_libraries(cloud PUBLIC nlohmann_json::nlohmann_json)
  target_compile_definitions(cloud PUBLIC USE_NLOHMANN_JSON=1)
endif()

# main executable
add_executable(register_mvp src/main.cpp)
target_link_libraries(register_mvp PRIVATE app drivers hal util cfg server cloud)
target_include_directories(register_mvp PRIVATE . src)

enable_testing()
add_subdirectory(tests)

