#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/../.." && pwd)"
COMPILER="$PROJECT_DIR/build/ccompiler"

if [ $# -lt 1 ]; then
    echo "usage: $0 <source.c> [expected_exit_code]"
    exit 1
fi

SOURCE="$1"
BASENAME="$(basename "$SOURCE" .c)"

# Extract expected exit code from filename: test_<name>__<expected>.c
# An explicit second arg overrides the filename-derived value.
if [ $# -ge 2 ]; then
    EXPECTED="$2"
elif [[ "$BASENAME" == *__* ]]; then
    EXPECTED="${BASENAME##*__}"
else
    EXPECTED=""
fi

# Compile C -> ASM
echo "compiling: $SOURCE"
"$COMPILER" "$SOURCE"

# Assemble ASM -> object
echo "assembling: ${BASENAME}.asm"
nasm -f elf64 "${BASENAME}.asm" -o "${BASENAME}.o"

# Link object -> executable
echo "linking: ${BASENAME}"
ld "${BASENAME}.o" -o "${BASENAME}" -e main

# Run
echo "running: ./${BASENAME}"
STDOUT_FILE="${BASENAME}.stdout"
set +e
"./${BASENAME}" >"$STDOUT_FILE"
ACTUAL=$?
set -e

echo "exit code: $ACTUAL"

# Check expected exit code if provided
if [ -n "$EXPECTED" ]; then
    if [ "$ACTUAL" -eq "$EXPECTED" ]; then
        echo "PASS (expected $EXPECTED)"
    else
        echo "FAIL (expected $EXPECTED, got $ACTUAL)"
        exit 1
    fi
fi

# Optional stdout comparison. If a sibling .expected file exists, the
# captured stdout must match it byte-for-byte.
SOURCE_DIR="$(cd "$(dirname "$SOURCE")" && pwd)"
EXPECTED_FILE="$SOURCE_DIR/${BASENAME}.expected"
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
