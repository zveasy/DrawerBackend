FROM debian:bookworm AS build

RUN apt-get update \
    && apt-get install -y --no-install-recommends \
       build-essential cmake pkg-config libgpiod-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY . .

ARG USE_MOCK_GPIO=ON
RUN cmake -S . -B build -DUSE_MOCK_GPIO=${USE_MOCK_GPIO} \
    && cmake --build build -j

CMD ["./build/register_mvp"]
