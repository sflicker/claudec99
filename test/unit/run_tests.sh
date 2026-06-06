#!/bin/bash
#
# Unit test runner for the Vec module.
# Compiles test_vec.c with the system C compiler and runs it.
# Outputs a "Results: P passed, F failed, T total" line for aggregation.
#

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(cd "${SCRIPT_DIR}/../.." && pwd)"

TMP_BIN="${SCRIPT_DIR}/test_vec_bin"

# Compile the test.
if ! gcc -std=c99 -Wall -Wextra -pedantic \
        -I "${PROJECT_DIR}/include" \
        "${PROJECT_DIR}/src/vec.c" \
        "${SCRIPT_DIR}/test_vec.c" \
        -o "${TMP_BIN}" 2>&1; then
    echo "FAIL  unit/test_vec: compilation failed"
    echo "Results: 0 passed, 1 failed, 1 total"
    exit 1
fi

# Run the test binary.
"${TMP_BIN}"
rc=$?

rm -f "${TMP_BIN}"
exit $rc
