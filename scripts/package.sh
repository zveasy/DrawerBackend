#!/usr/bin/env bash
set -e
rm -rf build
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DUSE_MOCK_GPIO=OFF -DBUILD_TESTING=OFF
cmake --build build -j
cpack --config build/CPackConfig.cmake -G DEB
echo "Artifacts:"
ls -lh *.deb build/*.deb || true
