#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
# Toolchain installed locally in this managed cloud machine.
export PKG_CONFIG_SYSROOT_DIR=/workspace/toolchain/sysroot
export PKG_CONFIG_LIBDIR="$PKG_CONFIG_SYSROOT_DIR/usr/lib/x86_64-linux-gnu/pkgconfig:$PKG_CONFIG_SYSROOT_DIR/usr/share/pkgconfig"
export CPLUS_INCLUDE_PATH="$PKG_CONFIG_SYSROOT_DIR/usr/include:$PKG_CONFIG_SYSROOT_DIR/usr/include/x86_64-linux-gnu"
cmake_bin=/workspace/toolchain/python/cmake/data/bin/cmake
"$cmake_bin" -S . -B build -G Ninja \
  -DCMAKE_MAKE_PROGRAM=/workspace/toolchain/python/bin/ninja \
  -DJUCE_SOURCE_DIR=/workspace/juce -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_LIBRARY_PATH="$PKG_CONFIG_SYSROOT_DIR/usr/lib/x86_64-linux-gnu" \
  -DCMAKE_INCLUDE_PATH="$PKG_CONFIG_SYSROOT_DIR/usr/include" \
  -DCMAKE_EXE_LINKER_FLAGS="-L$PKG_CONFIG_SYSROOT_DIR/usr/lib/x86_64-linux-gnu" \
  -DCMAKE_SHARED_LINKER_FLAGS="-L$PKG_CONFIG_SYSROOT_DIR/usr/lib/x86_64-linux-gnu"
"$cmake_bin" --build build --parallel 2
