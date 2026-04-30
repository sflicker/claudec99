#!/bin/bash
#
# Test runner for emitted-.asm output (stage 14-03 onward).
# Compiles each test .c file in normal mode and compares the generated
# .asm file against the corresponding .expected file. The .asm is
# written into a per-suite work directory so the source tree stays
# clean.
#

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/../.." && pwd)"
COMPILER="$PROJECT_DIR/build/ccompiler"
WORK_DIR="$PROJECT_DIR/build/print_asm_tmp"

mkdir -p "$WORK_DIR"

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

    # Compile .c -> .asm. The compiler writes the .asm into the
    # current working directory using only the source basename, so
    # run from the work dir to keep the source tree clean.
    if ! ( cd "$WORK_DIR" && "$COMPILER" "$src" >/dev/null 2>&1 ); then
        echo "FAIL  $name  (compiler error)"
        fail=$((fail + 1))
        continue
    fi

    asm_file="$WORK_DIR/${name}.asm"
    if [ ! -f "$asm_file" ]; then
        echo "FAIL  $name  (no .asm produced)"
        fail=$((fail + 1))
        continue
    fi
    actual=$(cat "$asm_file")

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

# Cleanup
rm -rf "$WORK_DIR"

if [ "$fail" -ne 0 ]; then
    exit 1
fi
exit 0
