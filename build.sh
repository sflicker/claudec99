#!/bin/bash
#
# build.sh — Bootstrap-aware build driver for ClaudeC99
#
# Modes (--mode=):
#   normal    Use the system C compiler (GCC or Clang) via cmake.  Default.
#   bootstrap Use the pre-built ClaudeC99 compiler at build/ccompiler.
#             Requires a prior normal build to exist.
#   fallback  Use ClaudeC99 if build/ccompiler exists; otherwise normal.
#
# Options:
#   --timeout=N   Per-file compilation timeout in seconds for bootstrap
#                 builds (default: 300).
#
# Usage examples:
#   ./build.sh                          # normal cmake build
#   ./build.sh --mode=bootstrap         # self-hosted build
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
            echo "Usage: $0 [--mode=normal|bootstrap|fallback] [--timeout=N]" >&2
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
    src/type.c
    src/util.c
    src/version.c
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
    # Extract the dotted version number (e.g. "00.02.00920000") and convert
    # dots and spaces to underscores to form a valid C token.
    local version_num
    version_num=$(echo "$version_line" | grep -oE '[0-9]+\.[0-9]+\.[0-9]+' | head -1 | tr '.' '_')
    local builtby
    if [ -n "$version_num" ]; then
        builtby="ClaudeC99_v${version_num}"
    else
        builtby="ClaudeC99"
    fi
    echo "BuiltBy token: $builtby"

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
