#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")" && pwd)"

find "$ROOT" -maxdepth 1 \( -name "*.asm" -o -name "*.o" -o -name "a.out" \) -delete
find "$ROOT/benchmarks" \( -name "*.asm" -o -name "*.o" -o -name "a.out" \) -delete
find "$ROOT/test" \( -name "*.asm" -o -name "*.o" -o -name "a.out" \) -delete

if [[ "${1:-}" == "--build" ]]; then
  rm -rf "$ROOT/build" "$ROOT/cmake-build-debug"
fi
