# Convenience Makefile for local development

.DEFAULT_GOAL := build

BUILD_DIR := build
CONFIG_FLAGS := -DCMAKE_BUILD_TYPE=RelWithDebInfo

configure:
	cmake -S . -B $(BUILD_DIR) $(CONFIG_FLAGS)

build: | configure
	cmake --build $(BUILD_DIR) -j

rebuild:
	rm -rf $(BUILD_DIR)
	$(MAKE) build

# Run unit tests if they are enabled in CMake
 test:
	ctest --test-dir $(BUILD_DIR) --output-on-failure

# Dev service management
 dev-up: build
	./scripts/dev/start_all.sh

 dev-down:
	./scripts/dev/stop_all.sh

# Quick run helpers (foreground)
 api: build
	./$(BUILD_DIR)/register_mvp --api 8080

 docs: build
	REGISTER_MVP_DOCS_PATH="$(PWD)/docs" ./$(BUILD_DIR)/register_mvp --api 8082

 pos: build
	./$(BUILD_DIR)/register_mvp --pos-http 9090

 tui: build
	./$(BUILD_DIR)/register_mvp --tui 8081

# Logs
 tail-logs:
	./scripts/dev/tail_logs.sh

# Smoke tests against running dev services
 smoke:
	./scripts/dev/smoke_test.sh

# Cleanup
 clean-build:
	rm -rf $(BUILD_DIR)

.PHONY: configure build rebuild test dev-up dev-down api docs pos tui tail-logs smoke clean-build
