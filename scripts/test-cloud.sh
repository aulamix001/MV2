#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../build"
export XDG_CACHE_HOME=/workspace/toolchain/cache
mkdir -p "$XDG_CACHE_HOME/fontconfig"
export LD_LIBRARY_PATH="/workspace/toolchain/sysroot/usr/lib/x86_64-linux-gnu${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
if [[ -z "${DISPLAY:-}" ]]; then
  test_tmp=$(mktemp -d /tmp/mv2-tests.XXXXXX)
  /workspace/toolchain/sysroot/usr/bin/Xvfb -displayfd 3 -screen 0 1600x1200x24 -ac -nolisten tcp \
    -xkbdir /workspace/toolchain/sysroot/usr/share/X11/xkb 3>"$test_tmp/display" >"$test_tmp/xvfb.log" 2>&1 &
  xvfb_pid=$!
  trap 'kill "$xvfb_pid" 2>/dev/null || true; wait "$xvfb_pid" 2>/dev/null || true; rm -rf "$test_tmp"' EXIT
  for attempt in {1..50}; do
    [[ -s "$test_tmp/display" ]] && break
    if ! kill -0 "$xvfb_pid" 2>/dev/null; then cat "$test_tmp/xvfb.log"; exit 1; fi
    sleep 0.1
  done
  [[ -s "$test_tmp/display" ]] || { cat "$test_tmp/xvfb.log"; exit 1; }
  export DISPLAY=":$(cat "$test_tmp/display")"
fi
/workspace/toolchain/python/cmake/data/bin/ctest --output-on-failure
