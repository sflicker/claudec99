#!/bin/bash
# Stage 180: verify target-dependent predefined macros for each explicit
# --target, independent of the host.  --print-tokens shows the token stream
# after preprocessing, so only the identifiers whose #if defined(...) guard
# held appear in the output.

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/../../.." && pwd)"
# Stage 180: host/target settings (TARGET, NASM_FMT, LINK_CMD, run_timeout).
. "$PROJECT_DIR/test/lib/platform.sh"
COMPILER="$PROJECT_DIR/build/ccompiler"

idents() {   # usage: idents <target> -> space-separated surviving identifiers
    run_timeout 30 "$COMPILER" "--target=$1" --print-tokens "$SCRIPT_DIR/macros.c" \
        | awk '/TOKEN_IDENTIFIER/ { for (i = 1; i < NF; i++) if ($i == "TOKEN::") { printf "%s ", $(i + 1); break } }'
}

check() {    # usage: check <target> <expected>
    local actual
    actual="$(idents "$1")"
    if [ "$actual" != "$2" ]; then
        echo "FAIL: --target=$1: expected '$2', got '$actual'" >&2
        exit 1
    fi
}

check x86_64-linux "has_linux has_unix has_x86_64 "
check x86_64-macos "has_apple has_mach has_x86_64 "
exit 0
