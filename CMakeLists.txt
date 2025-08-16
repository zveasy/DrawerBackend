cmake_minimum_required(VERSION 3.16)
project(register_mvp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

option(USE_MOCK_GPIO "Build with a mock GPIO HAL (no libgpiod)" ON)
option(USE_AWS_SDK "Build with AWS IoT CRT MQTT client" OFF)
option(USE_NLOHMANN_JSON "Use nlohmann/json single header" ON)
option(ENABLE_SERIAL_POS "Build serial POS connector stub" OFF)
option(ENABLE_OBSERVABILITY "Enable /metrics endpoints" ON)

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
  src/util/event_log.cpp
)
target_include_directories(util PUBLIC . src)

# observability
add_library(obs
  src/obs/metrics.cpp
  src/obs/metrics_endpoint.cpp
  src/obs/sli_slo.cpp
)
target_include_directories(obs PUBLIC . src)
target_link_libraries(obs PUBLIC httplib::httplib pthread)

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
  target_link_libraries(hal PRIVATE ${GPIOD_LIBRARIES})
endif()

# drivers
add_library(drivers
  src/drivers/stepper.cpp
  src/drivers/hopper_parallel.cpp
  src/drivers/hx711.cpp
)
target_include_directories(drivers PUBLIC . src)
target_link_libraries(drivers PUBLIC hal)

# safety
add_library(safety
  src/safety/faults.cpp
)
target_include_directories(safety PUBLIC . src)
target_link_libraries(safety PUBLIC hal util)

# app
add_library(app
  src/app/shutter_fsm.cpp
  src/app/dispense_ctrl.cpp
  src/app/audit.cpp
  src/app/shutter_adapter.cpp
  src/app/dispense_adapter.cpp
  src/app/txn_engine.cpp
  src/app/selftest.cpp
  src/app/inventory.cpp
  src/app/multi_dispenser.cpp
  src/app/change_maker.cpp
  src/app/service_mode.cpp
  src/app/jam_clear.cpp
)
target_include_directories(app PUBLIC . src)
target_link_libraries(app PUBLIC drivers hal util safety)

# server / api
add_library(server
  src/server/http_server.cpp
  src/ui/tui.cpp
)
target_include_directories(server PUBLIC . src)
target_link_libraries(server PUBLIC app httplib::httplib pthread obs)

# pos connectors
add_library(pos
  src/pos/http_pos.cpp
)
target_include_directories(pos PUBLIC . src)
target_link_libraries(pos PUBLIC app httplib::httplib pthread)
if(ENABLE_SERIAL_POS)
  target_sources(pos PRIVATE src/pos/serial_pos.cpp)
  target_compile_definitions(pos PRIVATE ENABLE_SERIAL_POS)
endif()

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
  src/cloud/tls_identity.cpp
  src/cloud/identity.cpp
)
target_include_directories(cloud PUBLIC . src)
target_link_libraries(cloud PUBLIC util)
if(USE_NLOHMANN_JSON)
  target_link_libraries(cloud PUBLIC nlohmann_json::nlohmann_json)
  target_compile_definitions(cloud PUBLIC USE_NLOHMANN_JSON=1)
endif()

# main executable
add_executable(register_mvp src/main.cpp)
target_link_libraries(register_mvp PRIVATE app drivers hal util cfg server cloud safety pos obs)
target_include_directories(register_mvp PRIVATE . src)

if(BUILD_TESTING)
  enable_testing()
  add_subdirectory(tests)
endif()

include(GNUInstallDirs)

# Binaries
install(TARGETS register_mvp RUNTIME DESTINATION /opt/register_mvp/bin)

# Example config
install(FILES config/config.example.ini DESTINATION /opt/register_mvp/share RENAME config.ini.example)

# Systemd + logrotate
install(FILES packaging/systemd/register-mvp.service DESTINATION /lib/systemd/system)
install(FILES packaging/logrotate/register-mvp DESTINATION /etc/logrotate.d)
install(FILES packaging/tmpfiles.d/register-mvp.conf DESTINATION /usr/lib/tmpfiles.d)
install(FILES packaging/firewall/nftables.conf DESTINATION /opt/register_mvp/packaging)
install(FILES packaging/sshd/sshd_hardening.conf DESTINATION /opt/register_mvp/packaging)
install(FILES packaging/auditd/audit.rules DESTINATION /opt/register_mvp/packaging)

# Postinst/prerm
install(PROGRAMS scripts/postinst.sh DESTINATION /opt/register_mvp/packaging)
install(PROGRAMS scripts/prerm.sh   DESTINATION /opt/register_mvp/packaging)
install(PROGRAMS scripts/provision/provision_device.sh DESTINATION /opt/register_mvp/packaging)
install(PROGRAMS scripts/rotate/rotate_secrets.sh DESTINATION /opt/register_mvp/packaging)
install(PROGRAMS scripts/hardening/apply_hardening.sh DESTINATION /opt/register_mvp/packaging)
install(PROGRAMS scripts/hardening/ro_root_overlay.sh DESTINATION /opt/register_mvp/packaging)
install(PROGRAMS scripts/audit/security_audit.sh DESTINATION /opt/register_mvp/packaging)

install(DIRECTORY docs/ DESTINATION /opt/register_mvp/share)

# CPack metadata
set(CPACK_GENERATOR "DEB")
set(CPACK_PACKAGE_NAME "register-mvp")
set(CPACK_PACKAGE_VENDOR "YourCompany")
set(CPACK_PACKAGE_CONTACT "ops@yourcompany.com")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Register MVP with POS connector and local API")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "YourCompany")
set(CPACK_PACKAGE_VERSION "1.0.0")
include(CPack)
