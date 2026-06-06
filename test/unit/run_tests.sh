#!/bin/bash
#
# Unit test runner for compiler utility modules (Vec, StrBuf, ...).
# Compiles and runs each test binary with the system C compiler.
# Outputs a combined "Results: P passed, F failed, T total" line.
#

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(cd "${SCRIPT_DIR}/../.." && pwd)"

total_pass=0
total_fail=0
overall_rc=0

run_unit_test() {
    local name="$1"       # display name
    local src="$2"        # test source file (absolute)
    local deps="$3"       # space-separated extra source files
    local bin="${SCRIPT_DIR}/${name}_bin"

    # Build compile command.
    # shellcheck disable=SC2086
    if ! gcc -std=c99 -Wall -Wextra -pedantic \
            -I "${PROJECT_DIR}/include" \
            $deps \
            "$src" \
            -o "$bin" 2>&1; then
        echo "FAIL  unit/${name}: compilation failed"
        total_fail=$((total_fail + 1))
        overall_rc=1
        return
    fi

    output=$("$bin")
    rc=$?
    echo "$output"
    rm -f "$bin"

    if [ $rc -ne 0 ]; then
        overall_rc=1
    fi

    # Parse "Results: P passed, F failed, T total" from last line.
    summary=$(echo "$output" | tail -n 1)
    if [[ "$summary" =~ Results:\ ([0-9]+)\ passed,\ ([0-9]+)\ failed,\ ([0-9]+)\ total ]]; then
        total_pass=$((total_pass + ${BASH_REMATCH[1]}))
        total_fail=$((total_fail + ${BASH_REMATCH[2]}))
    else
        echo "WARN  could not parse summary for ${name}"
        overall_rc=1
    fi
}

run_unit_test "test_vec" \
    "${SCRIPT_DIR}/test_vec.c" \
    "${PROJECT_DIR}/src/vec.c"

run_unit_test "test_strbuf" \
    "${SCRIPT_DIR}/test_strbuf.c" \
    "${PROJECT_DIR}/src/strbuf.c"

total=$((total_pass + total_fail))
echo ""
echo "Results: ${total_pass} passed, ${total_fail} failed, ${total} total"
exit $overall_rc
