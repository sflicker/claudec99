#!/bin/bash
#
# Top-level test runner: invokes every per-suite run_tests.sh and
# aggregates the results so a single command surfaces the
# project-wide pass/fail counts.
#

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

suites=(
    valid
    invalid
    print_ast
    print_tokens
)

agg_pass=0
agg_fail=0
agg_total=0
overall_rc=0

for suite in "${suites[@]}"; do
    runner="$SCRIPT_DIR/$suite/run_tests.sh"
    if [ ! -x "$runner" ]; then
        echo "SKIP  suite '$suite'  (no executable runner at $runner)"
        continue
    fi

    echo "===================================================="
    echo "Running suite: $suite"
    echo "===================================================="
    output=$("$runner" 2>&1)
    rc=$?
    echo "$output"

    # Final line of every suite runner is:
    #   "Results: P passed, F failed, T total"
    summary=$(echo "$output" | tail -n 1)
    if [[ "$summary" =~ Results:\ ([0-9]+)\ passed,\ ([0-9]+)\ failed,\ ([0-9]+)\ total ]]; then
        p="${BASH_REMATCH[1]}"
        f="${BASH_REMATCH[2]}"
        t="${BASH_REMATCH[3]}"
        agg_pass=$((agg_pass + p))
        agg_fail=$((agg_fail + f))
        agg_total=$((agg_total + t))
    else
        echo "WARN  could not parse summary line for '$suite'"
    fi

    if [ "$rc" -ne 0 ]; then
        overall_rc=1
    fi
    echo ""
done

echo "===================================================="
echo "Aggregate: $agg_pass passed, $agg_fail failed, $agg_total total"
echo "===================================================="

exit $overall_rc
