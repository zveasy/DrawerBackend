# syntax=docker/dockerfile:1.7
FROM debian:bookworm AS build

# Leverage BuildKit cache for apt
RUN --mount=type=cache,id=apt-cache,target=/var/cache/apt,sharing=locked \
    --mount=type=cache,id=apt-lib,target=/var/lib/apt,sharing=locked \
    apt-get update \
    && apt-get install -y --no-install-recommends \
       build-essential cmake pkg-config git libgpiod-dev libssl-dev ca-certificates curl ccache \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY . .

ARG USE_MOCK_GPIO=ON

# Enable compiler caching via ccache
ENV CCACHE_DIR=/root/.cache/ccache \
    CCACHE_MAXSIZE=500M \
    CCACHE_COMPRESS=1

# Configure and build with cache mounts for ccache
RUN --mount=type=cache,target=/root/.cache/ccache \
    rm -rf build \
    && cmake -S . -B build -DUSE_MOCK_GPIO=${USE_MOCK_GPIO} -DCMAKE_C_COMPILER_LAUNCHER=ccache -DCMAKE_CXX_COMPILER_LAUNCHER=ccache \
    && cmake --build build -j

CMD ["./build/register_mvp"]
