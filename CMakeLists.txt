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
option(ENABLE_QUANT "Enable QuantEngine bridge" OFF)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  add_compile_options(-Wall -Wextra -Wpedantic -O2 -g)
endif()

find_package(OpenSSL REQUIRED)
find_package(Threads REQUIRED)

set(APP_VERSION "1.0.0")
execute_process(COMMAND git rev-parse --short HEAD OUTPUT_VARIABLE GIT_SHA OUTPUT_STRIP_TRAILING_WHITESPACE)
add_compile_definitions(APP_VERSION="${APP_VERSION}" GIT_COMMIT="${GIT_SHA}")

include(FetchContent)
FetchContent_Declare(cpphttplib
  GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
  GIT_TAG v0.15.3)
FetchContent_MakeAvailable(cpphttplib)
add_library(httplib::httplib ALIAS httplib)

# On macOS, cpp-httplib may enable loading certs from the system keychain which
# pulls in <Security/Security.h> and clashes with our `namespace audit` symbol name.
# Remove CPPHTTPLIB_USE_CERTS_FROM_MACOSX_KEYCHAIN from the interface definitions.
get_target_property(_httplib_defs httplib INTERFACE_COMPILE_DEFINITIONS)
if(_httplib_defs)
  list(REMOVE_ITEM _httplib_defs CPPHTTPLIB_USE_CERTS_FROM_MACOSX_KEYCHAIN)
  set_target_properties(httplib PROPERTIES INTERFACE_COMPILE_DEFINITIONS "${_httplib_defs}")
endif()

if(USE_NLOHMANN_JSON)
  FetchContent_Declare(nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.2)
  FetchContent_MakeAvailable(nlohmann_json)
endif()

if(ENABLE_QUANT)
  FetchContent_Declare(libzmq
    GIT_REPOSITORY https://github.com/zeromq/libzmq.git
    GIT_TAG v4.3.5)
  FetchContent_Declare(cppzmq
    GIT_REPOSITORY https://github.com/zeromq/cppzmq.git
    GIT_TAG v4.10.0)
  FetchContent_MakeAvailable(libzmq cppzmq)
endif()

# util
add_library(util
  util/log.cpp
  src/util/persist.cpp
  src/util/journal.cpp
  src/util/metrics.cpp
  src/util/fs.cpp
  src/util/event_log.cpp
  src/util/version.cpp
  src/util/ed25519_verify.cpp
  src/util/crypto_hmac.cpp
)
target_include_directories(util PUBLIC . src)
target_link_libraries(util PUBLIC OpenSSL::Crypto)

# observability
add_library(obs
  src/obs/metrics.cpp
  src/obs/metrics_endpoint.cpp
  src/obs/sli_slo.cpp
)
target_include_directories(obs PUBLIC . src)
target_link_libraries(obs PUBLIC httplib::httplib Threads::Threads)

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

# compliance
add_library(compliance
  src/compliance/compliance_mode.cpp
)
target_include_directories(compliance PUBLIC . src)
target_link_libraries(compliance PUBLIC util safety)

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

# manufacturing helpers
add_library(mfg
  src/eol/eol_spec.cpp
  src/eol/eol_runner.cpp
  src/provision/serial.cpp
  src/burnin/burnin.cpp
)
target_include_directories(mfg PUBLIC . src)
target_link_libraries(mfg PUBLIC app drivers util cfg cloud)

# server / api
add_library(server
  src/server/http_server.cpp
  src/server/version_endpoint.cpp
  src/server/eol_endpoint.cpp
  src/server/docs_endpoint.cpp
  src/ui/tui.cpp
)
target_include_directories(server PUBLIC . src)
target_link_libraries(server PUBLIC app httplib::httplib Threads::Threads obs mfg OpenSSL::SSL OpenSSL::Crypto)
if(USE_NLOHMANN_JSON)
  target_link_libraries(server PUBLIC nlohmann_json::nlohmann_json)
  target_compile_definitions(server PUBLIC USE_NLOHMANN_JSON=1)
endif()
target_compile_definitions(server PUBLIC CPPHTTPLIB_OPENSSL_SUPPORT)
target_compile_options(server PRIVATE -UCPPHTTPLIB_USE_CERTS_FROM_MACOSX_KEYCHAIN)

# ota
add_library(ota
  src/ota/backend_local.cpp
  src/ota/backend_rauc.cpp
  src/ota/agent.cpp
  src/ota/healthd.cpp
)
target_include_directories(ota PUBLIC . src)
target_link_libraries(ota PUBLIC util cfg httplib::httplib OpenSSL::SSL OpenSSL::Crypto)
target_compile_definitions(ota PUBLIC CPPHTTPLIB_OPENSSL_SUPPORT)
target_compile_options(ota PRIVATE -UCPPHTTPLIB_USE_CERTS_FROM_MACOSX_KEYCHAIN)

# pos connectors
add_library(pos
  src/pos/http_pos.cpp
  src/pos/serial_pos.cpp
  src/pos/router.cpp
  src/pos/contracts.cpp
  src/pos/idempotency_store.cpp
  src/pos/vendors/vendor_a_adapter.cpp
  src/pos/vendors/vendor_b_adapter.cpp
)
target_include_directories(pos PUBLIC . src)
target_link_libraries(pos PUBLIC app httplib::httplib Threads::Threads nlohmann_json::nlohmann_json OpenSSL::SSL OpenSSL::Crypto)
target_compile_definitions(pos PUBLIC CPPHTTPLIB_OPENSSL_SUPPORT)
target_compile_options(pos PRIVATE -UCPPHTTPLIB_USE_CERTS_FROM_MACOSX_KEYCHAIN)

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

# quant
add_library(quant
  src/quant/ledger.cpp
  src/quant/publisher.cpp
)
target_include_directories(quant PUBLIC . src)
target_link_libraries(quant PUBLIC util)
if(USE_NLOHMANN_JSON)
  target_link_libraries(quant PUBLIC nlohmann_json::nlohmann_json)
  target_compile_definitions(quant PUBLIC USE_NLOHMANN_JSON=1)
endif()
if(ENABLE_QUANT)
  target_link_libraries(quant PUBLIC libzmq cppzmq)
  target_compile_definitions(quant PUBLIC ENABLE_QUANT=1)
endif()

# main executable
add_executable(register_mvp src/main.cpp)
target_link_libraries(register_mvp PRIVATE app drivers hal util cfg server cloud safety pos obs compliance quant OpenSSL::SSL OpenSSL::Crypto)
target_compile_definitions(register_mvp PRIVATE CPPHTTPLIB_OPENSSL_SUPPORT)
target_compile_options(register_mvp PRIVATE -UCPPHTTPLIB_USE_CERTS_FROM_MACOSX_KEYCHAIN)
target_include_directories(register_mvp PRIVATE . src)

include(CTest)
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

install(DIRECTORY docs/ DESTINATION /opt/register_mvp/share/docs PATTERN "compliance" EXCLUDE PATTERN "wi" EXCLUDE PATTERN "dfm_dfa" EXCLUDE PATTERN "eol" EXCLUDE)
install(DIRECTORY docs/compliance/ DESTINATION /opt/register_mvp/share/compliance)
install(DIRECTORY scripts/compliance/ DESTINATION /opt/register_mvp/bin/compliance FILE_PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
install(DIRECTORY docs/wi docs/dfm_dfa docs/eol DESTINATION /opt/register_mvp/share/mfg)
install(DIRECTORY assets/ DESTINATION /opt/register_mvp/share/docs/assets)
install(PROGRAMS scripts/support/collect_support_bundle.sh scripts/support/redact_bundle.py DESTINATION /opt/register_mvp/bin/support)
install(PROGRAMS scripts/demo/smoke_store_demo.sh DESTINATION /opt/register_mvp/bin/demo)
install(PROGRAMS scripts/provision/provision_unit.sh scripts/provision/print_label.py scripts/eol/run_eol.py scripts/eol/collect_reports.py DESTINATION /opt/register_mvp/bin/mfg)
install(FILES packaging/systemd/register-mvp-eol.service packaging/systemd/register-mvp-eol.path DESTINATION /lib/systemd/system)

# CPack metadata
set(CPACK_GENERATOR "DEB")
set(CPACK_PACKAGE_NAME "register-mvp")
set(CPACK_PACKAGE_VENDOR "YourCompany")
set(CPACK_PACKAGE_CONTACT "ops@yourcompany.com")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Register MVP with POS connector and local API")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "YourCompany")
set(CPACK_PACKAGE_VERSION "1.0.0")

# Dev convenience targets to manage local services
add_custom_target(dev-up
  COMMAND ${CMAKE_COMMAND} -E env REGISTER_MVP_DOCS_PATH=${CMAKE_SOURCE_DIR}/docs ${CMAKE_SOURCE_DIR}/scripts/dev/start_all.sh
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  USES_TERMINAL
)

add_custom_target(dev-down
  COMMAND ${CMAKE_SOURCE_DIR}/scripts/dev/stop_all.sh
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  USES_TERMINAL
)

include(CPack)
