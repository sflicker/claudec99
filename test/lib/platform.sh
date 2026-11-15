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
#   NASM_FMT                 nasm -f format for the x86 targets
#   ASM_EXT                  compiler output extension: asm (x86) | s (arm64)
#   LINK_CMD                 linker command array for $TARGET
#   ARM64_TIER               test tier for TARGET=arm64-macos (env override; default 1)
#   assemble <in> <out> [f…] assemble one compiler output file to an object
#   arm64_tier_includes <p>  0 if test/valid-relative path <p> is in ARM64_TIER scope
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
    x86_64-linux) NASM_FMT=elf64;   ASM_EXT=asm; LINK_CMD=(gcc -no-pie) ;;
    x86_64-macos) NASM_FMT=macho64; ASM_EXT=asm; LINK_CMD=(clang -arch x86_64) ;;
    arm64-macos)
        # Stage 181: native ARM64 backend (Phase 1+).
        if [ "$HOST_OS" != "Darwin" ]; then
            echo "error: TARGET=arm64-macos requires a macOS host" >&2; exit 1
        fi
        NASM_FMT=;      ASM_EXT=s;   LINK_CMD=(clang -arch arm64) ;;
    *)            echo "error: unknown TARGET '$TARGET'" >&2; exit 1 ;;
esac

# Stage 181: ARM64 test tier (see test/valid/arm64_tiers.txt).
ARM64_TIER=${ARM64_TIER:-1}

# Stage 181: assemble one compiler output file.
# usage: assemble <in> <out> [extra nasm flags...]
# Extra flags (e.g. NASM's -g -F dwarf) apply to the x86 targets only.
assemble() {
    local in="$1" out="$2"
    shift 2
    if [ "$TARGET" = "arm64-macos" ]; then
        clang -arch arm64 -c "$in" -o "$out"
    else
        nasm -f "$NASM_FMT" "$@" "$in" -o "$out"
    fi
}

# Stage 181: is a test/valid-relative path in scope for ARM64_TIER?
# Returns 0 when test/valid/arm64_tiers.txt has an entry with
# tier <= ARM64_TIER that equals the path or is a directory prefix of it.
arm64_tier_includes() {
    local rel="$1" manifest tier path
    manifest="$(dirname "${BASH_SOURCE[0]}")/../valid/arm64_tiers.txt"
    [ -f "$manifest" ] || return 1
    while read -r tier path; do
        case "$tier" in ''|'#'*) continue ;; esac
        [ "$tier" -le "$ARM64_TIER" ] 2>/dev/null || continue
        path="${path%/}"
        if [ "$rel" = "$path" ] || [ "${rel#"$path"/}" != "$rel" ]; then
            return 0
        fi
    done < "$manifest"
    return 1
}

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
