#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
COMPILER="$PROJECT_DIR/build/ccompiler"

if [ $# -lt 1 ]; then
    echo "usage: $0 <source.c> [expected_exit_code]"
    exit 1
fi

SOURCE="$1"
EXPECTED="${2:-}"
BASENAME="$(basename "$SOURCE" .c)"

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
set +e
"./${BASENAME}"
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

# Clean up
rm -f "${BASENAME}.asm" "${BASENAME}.o" "${BASENAME}"
