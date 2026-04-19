#!/bin/bash
#
# Test runner for claude99 compiler
# Compiles each test .c file, assembles, links, runs, and checks exit code == 42
#

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/../.." && pwd)"
COMPILER="$PROJECT_DIR/build/ccompiler"
WORK_DIR="$PROJECT_DIR/build/test_tmp"

mkdir -p "$WORK_DIR"

pass=0
fail=0
total=0

for src in "$SCRIPT_DIR"/*.c; do
    name=$(basename "$src" .c)
    total=$((total + 1))

    # Extract expected exit code from filename: test_<name>__<expected>.c
    if [[ "$name" == *__* ]]; then
        expected="${name##*__}"
    else
        echo "SKIP  $name  (no __<expected> suffix)"
        continue
    fi

    # Compile .c -> .asm
    if ! "$COMPILER" "$src" 2>/dev/null; then
        echo "FAIL  $name  (compiler error)"
        fail=$((fail + 1))
        continue
    fi

    asm_file="${name}.asm"

    # Move .asm into work dir
    mv "$asm_file" "$WORK_DIR/" 2>/dev/null

    # Assemble
    if ! nasm -f elf64 "$WORK_DIR/${name}.asm" -o "$WORK_DIR/${name}.o" 2>/dev/null; then
        echo "FAIL  $name  (nasm error)"
        fail=$((fail + 1))
        continue
    fi

    # Link — use main as the explicit entry point so helper definitions that
    # precede main in the translation unit do not become the entry.
    if ! ld -e main "$WORK_DIR/${name}.o" -o "$WORK_DIR/${name}" 2>/dev/null; then
        echo "FAIL  $name  (link error)"
        fail=$((fail + 1))
        continue
    fi

    # Run and check exit code
    "$WORK_DIR/${name}"
    actual=$?

    if [ "$actual" -eq "$expected" ]; then
        echo "PASS  $name  (exit code: $actual)"
        pass=$((pass + 1))
    else
        echo "FAIL  $name  (expected $expected, got $actual)"
        fail=$((fail + 1))
    fi
done

echo ""
echo "Results: $pass passed, $fail failed, $total total"

# Cleanup
rm -rf "$WORK_DIR"

if [ "$fail" -ne 0 ]; then
    exit 1
fi
exit 0
