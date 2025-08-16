# register_mvp

Sprint 1 & 2 — Repo, HAL, Sim Harness, and Shutter FSM.

The project provides a simple GPIO abstraction with mock and libgpiod backends,
logging helpers, a deterministic simulation harness, and a basic stepper-based
shutter controller with homing and soft-limit logic.

## Build

```bash
# default (mock HAL)
cmake -S . -B build -DUSE_MOCK_GPIO=ON
cmake --build build -j
./build/register_mvp --demo-shutter

# enable tests
ctest --test-dir build --output-on-failure

# optional: real GPIO backend (Linux)
sudo apt-get install -y libgpiod-dev
cmake -S . -B build-gpio -DUSE_MOCK_GPIO=OFF
cmake --build build-gpio -j
```
