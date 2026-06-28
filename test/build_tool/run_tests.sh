#!/bin/bash
#
# Build-tool compatibility test runner.
# Each subdirectory has a run_test.sh that drives make or cmake.
# An optional <name>.require file gates the test on a shell expression.
#
# Results line format (consumed by run_all_tests.sh):
#   Results: P passed, F failed, S skipped, T total
#

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/../.." && pwd)"
CC99="$PROJECT_DIR/bin/cc99"
WORK_DIR="$PROJECT_DIR/build/test_tmp_build_tool"
TIMEOUT=${CLAUDEC99_TEST_TIMEOUT:-60}

mkdir -p "$WORK_DIR"

pass=0
fail=0
skip=0
total=0

for test_dir in "$SCRIPT_DIR"/*/; do
    [ -d "$test_dir" ] || continue
    name=$(basename "$test_dir")
    total=$((total + 1))

    require_file="$test_dir/${name}.require"
    if [ -f "$require_file" ]; then
        req_cmd="$(cat "$require_file")"
        if ! eval "$req_cmd" >/dev/null 2>&1; then
            echo "SKIP  $name  (requires: $req_cmd)"
            skip=$((skip + 1))
            continue
        fi
    fi

    runner="$test_dir/run_test.sh"
    if [ ! -x "$runner" ]; then
        echo "SKIP  $name  (no executable run_test.sh)"
        skip=$((skip + 1))
        continue
    fi

    test_work="$WORK_DIR/$name"
    rm -rf "$test_work"
    cp -a "$test_dir" "$test_work"

    expected_file="$test_dir/${name}.expected"

    stdout_file="$test_work/${name}.stdout"
    stderr_file="$test_work/${name}.stderr"

    set +e
    (cd "$test_work" && timeout "$TIMEOUT" bash run_test.sh "$CC99" \
        >"$stdout_file" 2>"$stderr_file")
    rc=$?
    set -e

    if [ "$rc" -ne 0 ]; then
        echo "FAIL  $name  (exit $rc)"
        if [ -s "$stderr_file" ]; then
            head -5 "$stderr_file" | sed 's/^/      /' >&2
        fi
        fail=$((fail + 1))
        continue
    fi

    if [ -f "$expected_file" ]; then
        if ! diff -q "$expected_file" "$stdout_file" >/dev/null; then
            echo "FAIL  $name  (output mismatch)"
            diff "$expected_file" "$stdout_file" | head -10 >&2 || true
            fail=$((fail + 1))
            continue
        fi
    fi

    echo "PASS  $name"
    pass=$((pass + 1))
done

echo ""
echo "Results: $pass passed, $fail failed, $skip skipped, $total total"
