# DrawerBackend

Minimal C++17 + CMake project demonstrating a stepper-based shutter,
parallel hopper, and HX711 scale with a mockable GPIO HAL.

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


## Dev container

Open the folder in [VS Code Dev Containers](https://code.visualstudio.com/docs/remote/containers) or GitHub Codespaces and the Docker image will build automatically. The container maps `/dev/gpiochip0` and runs:

```
cmake -S . -B build -DUSE_MOCK_GPIO=ON && cmake --build build -j
```

so the project is ready to run:

```
./build/register_mvp
```

