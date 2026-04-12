#!/bin/bash
#
# Test runner for invalid test cases
# Compiles each test .c file and verifies the compiler rejects it
# with an error message matching the expected fragment from the filename.
#
# Filename format: test_<name>__<expected_error_fragment>.c
# Underscores in the fragment are treated as spaces when matching.
#

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/../.." && pwd)"
COMPILER="$PROJECT_DIR/build/ccompiler"

pass=0
fail=0
total=0

for src in "$SCRIPT_DIR"/test_*.c; do
    name=$(basename "$src" .c)
    total=$((total + 1))

    # Extract expected error fragment from filename: test_<name>__<fragment>.c
    if [[ "$name" == *__* ]]; then
        raw_fragment="${name##*__}"
        # Convert underscores to spaces for matching
        expected_fragment="${raw_fragment//_/ }"
    else
        echo "SKIP  $name  (no __<expected> suffix)"
        continue
    fi

    # Compile — should fail
    output=$("$COMPILER" "$src" 2>&1)
    rc=$?

    if [ "$rc" -eq 0 ]; then
        echo "FAIL  $name  (compiler should have rejected this, but succeeded)"
        fail=$((fail + 1))
        rm -f "${name}.asm"
        continue
    fi

    # Check that error message contains expected fragment (case-insensitive)
    if echo "$output" | grep -qi "$expected_fragment"; then
        echo "PASS  $name  (error contains: '$expected_fragment')"
        pass=$((pass + 1))
    else
        echo "FAIL  $name  (expected error containing '$expected_fragment', got: $output)"
        fail=$((fail + 1))
    fi
done

echo ""
echo "Results: $pass passed, $fail failed, $total total"

if [ "$fail" -ne 0 ]; then
    exit 1
fi
exit 0
