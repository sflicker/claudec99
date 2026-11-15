#!/bin/bash
#
# Stage 180: shared host/target settings for the test runners.
# Source this file (do not execute it):
#
#   . "$PROJECT_DIR/test/lib/platform.sh"
#
# Provides:
#   HOST_OS, HOST_ARCH       uname -s / uname -m
#   TARGET                   code generation target (override via env TARGET=...)
#   COMPILER_TARGET_FLAG     --target=$TARGET, passed to ccompiler
#   NASM_FMT                 nasm -f format for $TARGET
#   LINK_CMD                 linker command array for $TARGET
#   run_timeout <secs> cmd…  portable replacement for coreutils `timeout`
#

HOST_OS=$(uname -s)    # Linux | Darwin
HOST_ARCH=$(uname -m)  # x86_64 | arm64

# Code generation target. Override with TARGET=<name> in the environment.
if [ -z "$TARGET" ]; then
    if [ "$HOST_OS" = "Darwin" ]; then TARGET=x86_64-macos; else TARGET=x86_64-linux; fi
fi
COMPILER_TARGET_FLAG="--target=$TARGET"

case "$TARGET" in
    x86_64-linux) NASM_FMT=elf64;   LINK_CMD=(gcc -no-pie) ;;
    x86_64-macos) NASM_FMT=macho64; LINK_CMD=(clang -arch x86_64) ;;
    arm64-macos)  echo "error: TARGET=arm64-macos not supported until Stage 181" >&2; exit 1 ;;
    *)            echo "error: unknown TARGET '$TARGET'" >&2; exit 1 ;;
esac

# Portable timeout: timeout -> gtimeout -> perl alarm fallback.
# macOS ships no `timeout`. On expiry, coreutils timeout exits 124 while the
# perl fallback dies by SIGALRM (bash reports 142). No runner checks for a
# specific timeout status — a timed-out command is just a non-zero/unexpected
# exit — so the difference is harmless.
run_timeout() {   # usage: run_timeout <seconds> <cmd> [args...]
    if command -v timeout >/dev/null 2>&1; then timeout "$@"
    elif command -v gtimeout >/dev/null 2>&1; then gtimeout "$@"
    else perl -e 'alarm shift; exec @ARGV or exit 127' "$@"
    fi
}
