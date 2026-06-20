#!/bin/bash
#
# build.sh — Bootstrap-aware build driver for ClaudeC99
#
# Modes (--mode=):
#   normal     Use the system C compiler (GCC or Clang) via cmake.  Default.
#   bootstrap  Use the pre-built ClaudeC99 compiler at build/ccompiler.
#              Requires a prior normal build to exist.
#   fallback   Use ClaudeC99 if build/ccompiler exists; otherwise normal.
#   self-host  Full self-hosting test cycle (self-contained):
#              1. Archive existing ccompiler-c0/c1/c2 to build/previous/
#              2. Run cmake build to produce a fresh GCC-built ccompiler-c0
#              3. Bootstrap C0 → C1; run test suite; save as ccompiler-c1
#              4. Commit C1 checkpoint; bootstrap C1 → C2; run test suite; save as ccompiler-c2
#              build/ccompiler is left as C2 at the end.
#
# Options:
#   --timeout=N   Per-file compilation timeout in seconds for bootstrap
#                 builds (default: 300).
#
# Usage examples:
#   ./build.sh                          # normal cmake build
#   ./build.sh --mode=bootstrap         # single self-hosted build
#   ./build.sh --mode=self-host         # full C0→C1→C2 cycle with tests
#   ./build.sh --mode=fallback          # auto-select
#   ./build.sh --mode=bootstrap --timeout=60

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
MODE="normal"
BUILD_TIMEOUT=300

for arg in "$@"; do
    case "$arg" in
        --mode=*)    MODE="${arg#--mode=}" ;;
        --timeout=*) BUILD_TIMEOUT="${arg#--timeout=}" ;;
        *)
            echo "Unknown argument: $arg" >&2
            echo "Usage: $0 [--mode=normal|bootstrap|self-host|fallback] [--timeout=N]" >&2
            exit 1
            ;;
    esac
done

CCOMPILER_PATH="$SCRIPT_DIR/build/ccompiler"

# Source files that make up the compiler, in link order.
SRC_FILES=(
    src/compiler.c
    src/ast.c
    src/ast_pretty_printer.c
    src/codegen.c
    src/lexer.c
    src/parser.c
    src/preprocessor.c
    src/strbuf.c
    src/type.c
    src/util.c
    src/vec.c
    src/version.c
    src/optimize.c
    src/peephole.c
)

do_cmake_build() {
    cmake -S "$SCRIPT_DIR" -B "$SCRIPT_DIR/build"
    cmake --build "$SCRIPT_DIR/build"
}

do_bootstrap_build() {
    local ccompiler="$CCOMPILER_PATH"

    echo "Bootstrap build using: $ccompiler"

    # Derive a clean single-token BuiltBy value from the --version output.
    local version_line
    version_line=$(timeout 10 "$ccompiler" --version 2>&1 | head -1 || true)
    # Extract the full dotted version (e.g. "00.02.00940000.00656") — all four
    # groups including the build number — and convert dots to underscores.
    local version_num
    version_num=$(echo "$version_line" | grep -oE '[0-9]+\.[0-9]+\.[0-9]+\.[0-9]+' | head -1 | tr '.' '_')
    local builtby
    if [ -n "$version_num" ]; then
        builtby="ClaudeC99_v${version_num}"
    else
        builtby="ClaudeC99"
    fi
    echo "BuiltBy token: $builtby"

    # Compute the build number the same way cmake does (git commit count).
    local build_num
    build_num=$(git -C "$SCRIPT_DIR" rev-list --count HEAD 2>/dev/null || echo 0)
    echo "Build number: $build_num"

    local outdir="$SCRIPT_DIR/build/bootstrap_tmp"
    mkdir -p "$outdir"

    local obj_files=()

    for src in "${SRC_FILES[@]}"; do
        local name
        name=$(basename "$src" .c)
        local src_full="$SCRIPT_DIR/$src"
        local asm_file="$outdir/${name}.asm"
        local obj_file="$outdir/${name}.o"

        echo "  Compiling $src ..."
        # Run the compiler from the output dir so the .asm lands there.
        if ! (cd "$outdir" && timeout "$BUILD_TIMEOUT" "$ccompiler" \
                -I "$SCRIPT_DIR/include" \
                -I "$SCRIPT_DIR/test/include" \
                "-DVERSION_BUILTBY=${builtby}" \
                "-DVERSION_BUILD=${build_num}" \
                "$src_full"); then
            echo "FAIL: compilation failed for $src" >&2
            rm -rf "$outdir"
            return 1
        fi

        # Assemble.
        if ! nasm -f elf64 "$asm_file" -o "$obj_file"; then
            echo "FAIL: nasm failed for $name" >&2
            rm -rf "$outdir"
            return 1
        fi

        obj_files+=("$obj_file")
    done

    echo "  Linking ..."
    if ! gcc -no-pie "${obj_files[@]}" -o "$SCRIPT_DIR/build/ccompiler"; then
        echo "FAIL: link failed" >&2
        rm -rf "$outdir"
        return 1
    fi

    rm -rf "$outdir"
    echo "Bootstrap build complete."
}

do_self_host_test() {
    local prev_dir="$SCRIPT_DIR/build/previous"

    # Archive existing named copies so the current run starts clean.
    mkdir -p "$prev_dir"
    for name in ccompiler-c0 ccompiler-c1 ccompiler-c2; do
        if [ -f "$SCRIPT_DIR/build/$name" ]; then
            echo "Archiving build/$name -> build/previous/$name"
            mv "$SCRIPT_DIR/build/$name" "$prev_dir/$name"
        fi
    done

    # C0 must always be GCC-built. Run cmake now so it is, regardless of
    # what build/ccompiler currently contains.
    echo "=== Normal build (C0) ==="
    do_cmake_build
    cp "$CCOMPILER_PATH" "$SCRIPT_DIR/build/ccompiler-c0"
    echo ""
    echo "=== C0 (GCC-built) ==="
    "$SCRIPT_DIR/build/ccompiler-c0" --version
    echo "--- Running test suite with C0 ---"
    if ! "$SCRIPT_DIR/test/run_all_tests.sh"; then
        echo "FAIL: test suite failed with C0" >&2
        return 1
    fi

    # Commit after C0 is verified so C1's build number is strictly higher.
    echo ""
    echo "--- Committing C0 checkpoint ---"
    git -C "$SCRIPT_DIR" add -u
    git -C "$SCRIPT_DIR" commit --allow-empty -m "self-host C0 verified: all tests pass

Checkpoint commit so C1 build number exceeds C0.

Co-Authored-By: Claude Sonnet 4.6 <noreply@anthropic.com>"

    # C0 → C1
    echo ""
    echo "=== Bootstrap C0 → C1 ==="
    do_bootstrap_build
    cp "$CCOMPILER_PATH" "$SCRIPT_DIR/build/ccompiler-c1"
    echo "--- Running test suite with C1 ---"
    if ! "$SCRIPT_DIR/test/run_all_tests.sh"; then
        echo "FAIL: test suite failed with C1" >&2
        return 1
    fi

    # Commit after C1 is verified so C2's build number is strictly higher.
    # Stages any tracked-file changes first; falls back to an empty commit
    # when no source files changed (e.g. a clean pass with no bug fixes).
    echo ""
    echo "--- Committing C1 checkpoint ---"
    git -C "$SCRIPT_DIR" add -u
    git -C "$SCRIPT_DIR" commit --allow-empty -m "self-host C1 verified: all tests pass

Checkpoint commit so C2 build number exceeds C1.

Co-Authored-By: Claude Sonnet 4.6 <noreply@anthropic.com>"

    # C1 → C2
    echo ""
    echo "=== Bootstrap C1 → C2 ==="
    do_bootstrap_build
    cp "$CCOMPILER_PATH" "$SCRIPT_DIR/build/ccompiler-c2"
    echo "--- Running test suite with C2 ---"
    if ! "$SCRIPT_DIR/test/run_all_tests.sh"; then
        echo "FAIL: test suite failed with C2" >&2
        return 1
    fi

    echo ""
    echo "Self-host test complete."
    echo "  C0: $("$SCRIPT_DIR/build/ccompiler-c0" --version | head -1)"
    echo "  C1: $("$SCRIPT_DIR/build/ccompiler-c1" --version | head -1)"
    echo "  C2: $("$SCRIPT_DIR/build/ccompiler-c2" --version | head -1)"
}

case "$MODE" in
    normal)
        echo "=== Normal build ==="
        do_cmake_build
        ;;
    bootstrap)
        if [ ! -x "$CCOMPILER_PATH" ]; then
            echo "Error: bootstrap compiler not found at $CCOMPILER_PATH" >&2
            echo "Run a normal build first: ./build.sh --mode=normal" >&2
            exit 1
        fi
        echo "=== Bootstrap build ==="
        do_bootstrap_build
        ;;
    self-host)
        if [ ! -x "$CCOMPILER_PATH" ]; then
            echo "Error: build/ccompiler not found; run a normal build first." >&2
            exit 1
        fi
        echo "=== Self-host test ==="
        do_self_host_test
        ;;
    fallback)
        if [ -x "$CCOMPILER_PATH" ]; then
            echo "=== Fallback build: ClaudeC99 found, using bootstrap ==="
            do_bootstrap_build
        else
            echo "=== Fallback build: no ClaudeC99 found, using normal cmake build ==="
            do_cmake_build
        fi
        ;;
    *)
        echo "Error: unknown mode '$MODE' (valid: normal, bootstrap, fallback)" >&2
        exit 1
        ;;
esac
