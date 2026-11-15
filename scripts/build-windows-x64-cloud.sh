#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
export LD_LIBRARY_PATH="/workspace/toolchain/sysroot/usr/lib/x86_64-linux-gnu${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
export PATH="/workspace/toolchain/sysroot/usr/lib/llvm-19/bin:$PATH"
cmake_bin=/workspace/toolchain/python/cmake/data/bin/cmake
"$cmake_bin" -S . -B build-windows-clang-x64 -G Ninja \
  -DCMAKE_MAKE_PROGRAM=/workspace/toolchain/python/bin/ninja \
  -DCMAKE_TOOLCHAIN_FILE="$PWD/cmake/windows-x64-clang.cmake" \
  -DCMAKE_BUILD_TYPE=Release -DMV2_USE_INSTALLED_JUCE=ON \
  -DCMAKE_PREFIX_PATH=/workspace/toolchain/juce-sdk -DBUILD_TESTING=OFF
"$cmake_bin" --build build-windows-clang-x64 --parallel 3
