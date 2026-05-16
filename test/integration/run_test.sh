#!/bin/bash
#
# Single-test driver for the integration suite. Compiles all .c files in the
# test directory, assembles and links them with cc -no-pie, then runs.
#
# Each test lives in its own subdirectory: test/integration/<name>/
# Pass the main .c file: run_test.sh <name>/<name>.c
#
# Companion files (all optional, sharing the directory's basename):
#   BASENAME.expected  expected stdout (byte-for-byte compare)
#   BASENAME.libs      extra -l flags, one per token (e.g. "-lm")
#   BASENAME.args      command-line arguments passed when running
#   BASENAME.input     stdin redirected into the program
#   BASENAME.status    expected exit code (default 0 when absent)
#
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/../.." && pwd)"
COMPILER="$PROJECT_DIR/build/ccompiler"

if [ $# -lt 1 ]; then
    echo "usage: $0 <source.c>"
    exit 1
fi

SOURCE="$1"
SOURCE_DIR="$(cd "$(dirname "$SOURCE")" && pwd)"
BASENAME="$(basename "$SOURCE" .c)"

LIBS_FILE="$SOURCE_DIR/${BASENAME}.libs"
ARGS_FILE="$SOURCE_DIR/${BASENAME}.args"
INPUT_FILE="$SOURCE_DIR/${BASENAME}.input"
STATUS_FILE="$SOURCE_DIR/${BASENAME}.status"
EXPECTED_FILE="$SOURCE_DIR/${BASENAME}.expected"

if [ -f "$LIBS_FILE" ]; then
    EXTRA_LIBS=($(cat "$LIBS_FILE"))
else
    EXTRA_LIBS=()
fi

if [ -f "$ARGS_FILE" ]; then
    EXTRA_ARGS=($(cat "$ARGS_FILE"))
else
    EXTRA_ARGS=()
fi

if [ -f "$STATUS_FILE" ]; then
    EXPECTED_STATUS="$(cat "$STATUS_FILE")"
else
    EXPECTED_STATUS=0
fi

# Compile and assemble all .c files in the test directory
OBJ_FILES=()
for src in "$SOURCE_DIR"/*.c; do
    [ -f "$src" ] || continue
    src_name=$(basename "$src" .c)
    echo "compiling: $src"
    "$COMPILER" "$src"

    echo "assembling: ${src_name}.asm"
    nasm -f elf64 "${src_name}.asm" -o "${src_name}.o"

    OBJ_FILES+=("${src_name}.o")
done

# Link all objects -> executable
echo "linking: $BASENAME"
cc -no-pie "${OBJ_FILES[@]}" -o "$BASENAME" "${EXTRA_LIBS[@]}"

# Run with optional stdin redirection
echo "running: ./${BASENAME}"
STDOUT_FILE="${BASENAME}.stdout"
set +e
if [ -f "$INPUT_FILE" ]; then
    "./${BASENAME}" "${EXTRA_ARGS[@]}" <"$INPUT_FILE" >"$STDOUT_FILE"
else
    "./${BASENAME}" "${EXTRA_ARGS[@]}" >"$STDOUT_FILE"
fi
ACTUAL=$?
set -e

echo "exit code: $ACTUAL"

if [ "$ACTUAL" -eq "$EXPECTED_STATUS" ]; then
    echo "PASS (expected $EXPECTED_STATUS)"
else
    echo "FAIL (expected $EXPECTED_STATUS, got $ACTUAL)"
    exit 1
fi

if [ -f "$EXPECTED_FILE" ]; then
    if diff -q "$EXPECTED_FILE" "$STDOUT_FILE" >/dev/null; then
        echo "PASS (output matched)"
    else
        echo "FAIL (output mismatch)"
        diff "$EXPECTED_FILE" "$STDOUT_FILE" || true
        exit 1
    fi
fi

# Clean up all generated files
for src in "$SOURCE_DIR"/*.c; do
    [ -f "$src" ] || continue
    src_name=$(basename "$src" .c)
    rm -f "${src_name}.asm" "${src_name}.o"
done
rm -f "$BASENAME" "$STDOUT_FILE"
