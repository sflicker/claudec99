#!/bin/bash
# Test: per-file failure isolation.
# Invoke ccompiler with --max-errors=0 so an error in bad.c does not
# terminate the process; verify that:
#   1. The overall exit code is non-zero (bad.c failed).
#   2. good.asm is still produced (good.c was compiled despite the error).

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/../../.." && pwd)"
COMPILER="$PROJECT_DIR/build/ccompiler"
WORK="$PROJECT_DIR/build/test_tmp_integration/test_multi_file_error"
IFLAGS="-I${PROJECT_DIR}/test/include"

mkdir -p "$WORK"
cd "$WORK" || exit 1

# Expect non-zero exit (bad.c has a syntax error).
"$COMPILER" --max-errors=0 $IFLAGS \
    "$SCRIPT_DIR/bad.c" "$SCRIPT_DIR/good.c" 2>/dev/null
status=$?

if [ "$status" -eq 0 ]; then
    echo "FAIL: expected non-zero exit, got 0" >&2
    exit 1
fi

# good.asm must have been produced despite the earlier error.
if [ ! -f "good.asm" ]; then
    echo "FAIL: good.asm not produced after bad.c error" >&2
    exit 1
fi

exit 0
