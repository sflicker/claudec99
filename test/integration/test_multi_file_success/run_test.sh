#!/bin/bash
# Test: invoke ccompiler ONCE with two source files (file_a.c and file_b.c),
# then compile main.c separately, link all three, and verify output.
# Also verifies that both files with a same-named static symbol compile
# cleanly (independent parser/codegen state per file).

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/../../.." && pwd)"
COMPILER="$PROJECT_DIR/build/ccompiler"
WORK="$PROJECT_DIR/build/test_tmp_integration/test_multi_file_success"
IFLAGS="-I${PROJECT_DIR}/test/include"

mkdir -p "$WORK"
cd "$WORK" || exit 1

# Single invocation compiling two files at once.
if ! "$COMPILER" $IFLAGS \
        "$SCRIPT_DIR/file_a.c" "$SCRIPT_DIR/file_b.c" 2>/dev/null; then
    echo "FAIL: ccompiler rejected two-file invocation" >&2
    exit 1
fi

# Check both .asm files were produced in the working directory.
if [ ! -f "file_a.asm" ] || [ ! -f "file_b.asm" ]; then
    echo "FAIL: expected file_a.asm and file_b.asm" >&2
    exit 1
fi

# Compile main.c separately.
if ! "$COMPILER" $IFLAGS "$SCRIPT_DIR/main.c" 2>/dev/null; then
    echo "FAIL: could not compile main.c" >&2
    exit 1
fi

# Assemble all three.
for base in file_a file_b main; do
    if ! nasm -f elf64 "${base}.asm" -o "${base}.o" 2>/dev/null; then
        echo "FAIL: nasm failed for ${base}" >&2
        exit 1
    fi
done

# Link.
if ! cc -no-pie file_a.o file_b.o main.o \
        -o test_multi_file_success 2>/dev/null; then
    echo "FAIL: link failed" >&2
    exit 1
fi

# Run and check output.
actual=$(./test_multi_file_success)
expected="$(printf '42\n42\n')"
if [ "$actual" != "$expected" ]; then
    echo "FAIL: output mismatch (got: $actual, expected: $expected)" >&2
    exit 1
fi

exit 0
