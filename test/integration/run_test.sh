#!/bin/bash
#
# Single-test driver for the integration suite. Unlike test/valid, these
# tests link against libc (puts, strcmp, malloc, etc.), so the build step
# uses `cc -no-pie` and the linker pulls in crt0 + libc.
#
# Companion files (all optional, sharing the source's basename):
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

# Compile C -> ASM
echo "compiling: $SOURCE"
"$COMPILER" "$SOURCE"

# Assemble ASM -> object
echo "assembling: ${BASENAME}.asm"
nasm -f elf64 "${BASENAME}.asm" -o "${BASENAME}.o"

# Link object -> executable. cc -no-pie pulls in crt0 + libc.
echo "linking: ${BASENAME}"
cc -no-pie "${BASENAME}.o" -o "${BASENAME}" "${EXTRA_LIBS[@]}"

# Run with optional stdin redirection.
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

# Clean up
rm -f "${BASENAME}.asm" "${BASENAME}.o" "${BASENAME}" "$STDOUT_FILE"
