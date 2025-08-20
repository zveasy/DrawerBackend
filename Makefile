# Convenience Makefile for local development

.DEFAULT_GOAL := build

BUILD_DIR := build
CONFIG_FLAGS := -DCMAKE_BUILD_TYPE=RelWithDebInfo
DOCS_PORT ?= 8082

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

# Docker Compose helpers
compose-up:
	docker compose up -d

compose-down:
	docker compose down -v

compose-dev:
	docker compose --profile dev up -d

compose-dev-down:
	docker compose --profile dev down -v

docs-prod:
	docker compose --profile prod up docs_build
	docker compose --profile prod up -d docs_prod

docs-prod-down:
	docker compose --profile prod down -v

# Validation & quick checks
compose-validate:
	docker compose config -q

docs-prod-test:
	docker compose --profile prod up docs_build
	docker compose --profile prod up -d docs_prod
	for i in `seq 1 30`; do if curl -fsS http://localhost:$(DOCS_PORT)/ >/dev/null 2>&1; then echo OK; break; fi; sleep 1; done
	curl -fsS http://localhost:$(DOCS_PORT)/ | head -c 200 >/dev/null

docs-prod-logs:
	docker compose logs --no-color docs_prod

.PHONY: configure build rebuild test dev-up dev-down api docs pos tui tail-logs smoke clean-build compose-up compose-down compose-dev compose-dev-down docs-prod docs-prod-down compose-validate docs-prod-test docs-prod-logs
