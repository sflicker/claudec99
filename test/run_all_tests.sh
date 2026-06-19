#!/bin/bash
#
# Top-level test runner: invokes every per-suite run_tests.sh and
# aggregates the results so a single command surfaces the
# project-wide pass/fail counts.
#

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

suites=(
    unit
    valid
    invalid
    integration
    print_ast
    print_tokens
    print_asm
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
echo "Portable: $agg_pass passed, $agg_fail failed, $agg_total total"
echo "===================================================="

# On Linux x86_64 also run the system-include integration tests
if [[ "$(uname -s)" == "Linux" && "$(uname -m)" == "x86_64" ]]; then
    sysinclude_runner="$SCRIPT_DIR/integration/run_tests_sysinclude.sh"
    if [ -x "$sysinclude_runner" ]; then
        echo ""
        echo "===================================================="
        echo "Running suite: system include (Linux x86_64)"
        echo "===================================================="
        sys_output=$("$sysinclude_runner" 2>&1)
        sys_rc=$?
        echo "$sys_output"
        sys_summary=$(echo "$sys_output" | tail -n 1)
        if [[ "$sys_summary" =~ Results:\ ([0-9]+)\ passed,\ ([0-9]+)\ failed,\ ([0-9]+)\ total ]]; then
            sp="${BASH_REMATCH[1]}"
            sf="${BASH_REMATCH[2]}"
            st="${BASH_REMATCH[3]}"
        else
            sp=0; sf=0; st=0
            echo "WARN  could not parse summary line for system include suite"
        fi
        echo ""
        echo "===================================================="
        echo "System include: $sp passed, $sf failed, $st total"
        echo "===================================================="
    else
        echo "SKIP  system include suite (runner not found at $sysinclude_runner)"
    fi
fi

exit $overall_rc
