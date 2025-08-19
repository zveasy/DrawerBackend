
# register_mvp

Sprint 1 & 2 — Repo, HAL, Sim Harness, and Shutter FSM.

The project provides a simple GPIO abstraction with mock and libgpiod backends,
logging helpers, a deterministic simulation harness, and a basic stepper-based
shutter controller with homing and soft-limit logic.


# register_mvp

Sprint 1 — Repo, HAL, and Sim Harness.

The project provides a simple GPIO abstraction with mock and libgpiod backends,
logging helpers, and a deterministic simulation harness that exercises the
components without hardware.


## Build

```bash
# default (mock HAL)
cmake -S . -B build -DUSE_MOCK_GPIO=ON
cmake --build build -j

./build/register_mvp --demo-shutter

./build/register_mvp_sim --json


# enable tests
ctest --test-dir build --output-on-failure

# optional: real GPIO backend (Linux)
sudo apt-get install -y libgpiod-dev
cmake -S . -B build-gpio -DUSE_MOCK_GPIO=OFF
cmake --build build-gpio -j
```


# DrawerBackend

Minimal C++17 + CMake project demonstrating a stepper-based shutter,
parallel hopper, and HX711 scale with a mockable GPIO HAL.

## Architecture

See [docs/architecture.md](docs/architecture.md) for an overview of the HAL, drivers, application logic, HTTP server, and test harness.

## Native build

```bash
cmake -S . -B build -DUSE_MOCK_GPIO=ON
cmake --build build -j
./build/register_mvp
```

Pass `-DUSE_MOCK_GPIO=OFF` to use libgpiod on hardware (requires `libgpiod-dev`).

## Docker

Build and run with mock GPIO (works on any host that has Docker):

```bash
docker build -t register_mvp .
docker run --rm register_mvp
```

To access real GPIO, build without the mock and map the gpiochip device:

```bash
docker build -t register_mvp --build-arg USE_MOCK_GPIO=OFF .
docker run --rm --device /dev/gpiochip0 register_mvp
```

For a persistent development container that works across operating systems, use
the included Docker Compose file:

```bash
docker compose up -d --build
docker compose exec drawerbackend bash
```

When you're finished, shut down the container with:

```bash
docker compose down
```



## Dev container

Open the folder in [VS Code Dev Containers](https://code.visualstudio.com/docs/remote/containers) or GitHub Codespaces and the Docker image will build automatically. The container maps `/dev/gpiochip0` and runs:

```
cmake -S . -B build -DUSE_MOCK_GPIO=ON && cmake --build build -j
```

so the project is ready to run:

```
./build/register_mvp
```

## Makefile quickstart

Common workflows are wrapped in a Makefile:

```bash
# configure + build
make build

# run unit tests (after configuring with -DBUILD_TESTING=ON)
make test

# start/stop all dev services (API:8080, Docs API:8082, POS:9090, TUI:8081)
make dev-up
make dev-down

# tail all service logs
make tail-logs

# run smoke tests against running services
make smoke
```

## Docs endpoint

The API serves docs under `/help`.

- Root is controlled by env var `REGISTER_MVP_DOCS_PATH` (defaults to `/opt/register_mvp/share/docs`).
- Index: `GET /help` lists available documents.
- Files: `GET /help/<name>` serves files directly from the docs root.
- Fallback: if a `.html` file is not found, the server will attempt the corresponding `.md` file. For example, `/help/index.html` will serve `index.md` if present.

## Smoke tests

With dev services running (via `make dev-up`), run:

```bash
make smoke
```

This verifies:

- API `/version` and `/status`
- Docs `/help` index and `/help/index.html` fallback to Markdown
- POS `/ping`


## Formatting and Linting

This project uses [pre-commit](https://pre-commit.com/) with `clang-format` and `clang-tidy` to keep the C++ codebase consistent and catch common mistakes.

### Setup

```bash
pip install pre-commit
pre-commit install
```

### Usage

Run all checks against the entire repository:

```bash
pre-commit run --all-files
```

Pre-commit will automatically format sources and report any issues from `clang-tidy`.
