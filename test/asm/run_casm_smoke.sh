#!/bin/bash
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(cd "${SCRIPT_DIR}/../.." && pwd)"
CASM="${PROJECT_DIR}/build/casm"

pass=0
fail=0
if [ ! -x "$CASM" ]; then
    echo "SKIP  test/asm: build/casm not built (run cmake --build build)"
    echo "Results: 0 passed, 0 failed, 0 total"
    exit 0
fi

shopt -s globstar nullglob
files=("${PROJECT_DIR}"/build/**/*.asm)
if [ ${#files[@]} -eq 0 ]; then
    echo "SKIP  test/asm: no *.asm files under build/ (run ./build.sh --mode=self-host first)"
    echo "Results: 0 passed, 0 failed, 0 total"
    exit 0
fi

for f in "${files[@]}"; do
    if "$CASM" "$f" > /dev/null 2>&1; then
        pass=$((pass + 1))
    else
        echo "FAIL  asm/$(basename "$f")"
        fail=$((fail + 1))
    fi
done

echo "Results: ${pass} passed, ${fail} failed, $((pass + fail)) total"
[ "$fail" -eq 0 ]
