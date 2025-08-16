cmake_minimum_required(VERSION 3.16)
project(register_mvp_tests LANGUAGES CXX)

include(CTest)
include(FetchContent)

FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)
FetchContent_MakeAvailable(googletest)
include(GoogleTest)

add_executable(hal_mock_test hal_mock_test.cpp ../hal/gpio_mock.cpp ../util/log.cpp)
target_include_directories(hal_mock_test PRIVATE ..)
target_compile_features(hal_mock_test PRIVATE cxx_std_17)
if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(hal_mock_test PRIVATE -Wall -Wextra -Wpedantic -O2 -g)
endif()
target_link_libraries(hal_mock_test gtest_main)

gtest_discover_tests(hal_mock_test)

add_executable(sim_script_test sim_script_test.cpp ../sim/sim.cpp ../util/log.cpp)
target_include_directories(sim_script_test PRIVATE ..)
target_compile_features(sim_script_test PRIVATE cxx_std_17)
if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(sim_script_test PRIVATE -Wall -Wextra -Wpedantic -O2 -g)
endif()
target_link_libraries(sim_script_test gtest_main)

gtest_discover_tests(sim_script_test)
