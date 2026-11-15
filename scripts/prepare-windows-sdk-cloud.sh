#!/usr/bin/env bash
set -euo pipefail
# xwin verifies Microsoft package checksums. Do not disable TLS/checksum validation.
/workspace/toolchain/xwin/xwin-0.10.0-x86_64-unknown-linux-musl/xwin \
  --accept-license --arch x86_64 --variant desktop \
  --cache-dir /workspace/toolchain/xwin-cache \
  splat --output /workspace/toolchain/windows-sdk
