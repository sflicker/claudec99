#!/bin/bash
# Test: multi-file compilation with designated initializers.
# globals_a.c defines a global struct with member designators;
# globals_b.c defines a global array with index designators.

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/../../.." && pwd)"
COMPILER="$PROJECT_DIR/build/ccompiler"
WORK="$PROJECT_DIR/build/test_tmp_integration/test_designated_init_multifile"
IFLAGS="-I${PROJECT_DIR}/test/include"

mkdir -p "$WORK"
cd "$WORK" || exit 1

# Compile globals_a.c and globals_b.c in a single invocation.
if ! "$COMPILER" $IFLAGS \
        "$SCRIPT_DIR/globals_a.c" "$SCRIPT_DIR/globals_b.c" 2>/dev/null; then
    echo "FAIL: ccompiler rejected globals_a.c globals_b.c" >&2
    exit 1
fi

# Compile main.c separately.
if ! "$COMPILER" $IFLAGS "$SCRIPT_DIR/main.c" 2>/dev/null; then
    echo "FAIL: could not compile main.c" >&2
    exit 1
fi

# Assemble all three.
for base in globals_a globals_b main; do
    if ! nasm -f elf64 "${base}.asm" -o "${base}.o" 2>/dev/null; then
        echo "FAIL: nasm failed for ${base}" >&2
        exit 1
    fi
done

# Link.
if ! cc -no-pie globals_a.o globals_b.o main.o \
        -o test_designated_init_multifile 2>/dev/null; then
    echo "FAIL: link failed" >&2
    exit 1
fi

# Run and check output.
actual=$(./test_designated_init_multifile)
expected="$(printf '480000\n45\n')"
if [ "$actual" != "$expected" ]; then
    echo "FAIL: output mismatch (got: '$actual', expected: '$expected')" >&2
    exit 1
fi

exit 0
