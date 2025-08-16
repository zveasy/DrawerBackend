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
./build/register_mvp_sim --json

# enable tests
ctest --test-dir build --output-on-failure

# optional: real GPIO backend (Linux)
sudo apt-get install -y libgpiod-dev
cmake -S . -B build-gpio -DUSE_MOCK_GPIO=OFF
cmake --build build-gpio -j
```
