#!/bin/bash
#
# Test runner for --print-tokens output
# Compiles each test .c file with --print-tokens and compares output
# against the corresponding .expected file.
#

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/../.." && pwd)"
COMPILER="$PROJECT_DIR/build/ccompiler"

pass=0
fail=0
total=0

for src in "$SCRIPT_DIR"/test_*.c; do
    name=$(basename "$src" .c)
    expected_file="$SCRIPT_DIR/${name}.expected"
    total=$((total + 1))

    if [ ! -f "$expected_file" ]; then
        echo "SKIP  $name  (no .expected file)"
        continue
    fi

    actual=$("$COMPILER" --print-tokens "$src" 2>&1)
    rc=$?

    if [ "$rc" -ne 0 ]; then
        echo "FAIL  $name  (compiler exited with code $rc)"
        fail=$((fail + 1))
        continue
    fi

    # Verify no .asm file was generated
    asm_file="${name}.asm"
    if [ -f "$asm_file" ]; then
        echo "FAIL  $name  (.asm file should not be generated with --print-tokens)"
        rm -f "$asm_file"
        fail=$((fail + 1))
        continue
    fi

    expected=$(cat "$expected_file")
    if [ "$actual" = "$expected" ]; then
        echo "PASS  $name"
        pass=$((pass + 1))
    else
        echo "FAIL  $name  (output mismatch)"
        echo "  --- expected ---"
        echo "$expected"
        echo "  --- actual ---"
        echo "$actual"
        echo "  --- diff ---"
        diff <(echo "$expected") <(echo "$actual") || true
        fail=$((fail + 1))
    fi
done

echo ""
echo "Results: $pass passed, $fail failed, $total total"

if [ "$fail" -ne 0 ]; then
    exit 1
fi
exit 0
