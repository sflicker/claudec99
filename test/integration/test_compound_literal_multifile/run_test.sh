#!/bin/bash
# Test: multi-file compilation with compound literals.
# shapes.c defines rect_area(struct Rect); main.c passes compound literals.

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/../../.." && pwd)"
# Stage 180: host/target settings (TARGET, NASM_FMT, LINK_CMD, run_timeout).
. "$PROJECT_DIR/test/lib/platform.sh"
COMPILER="$PROJECT_DIR/build/ccompiler"
WORK="$PROJECT_DIR/build/test_tmp_integration/test_compound_literal_multifile"

mkdir -p "$WORK"
cd "$WORK" || exit 1

if ! "$COMPILER" "$COMPILER_TARGET_FLAG" "$SCRIPT_DIR/shapes.c" 2>/dev/null; then
    echo "FAIL: ccompiler rejected shapes.c" >&2
    exit 1
fi

if ! "$COMPILER" "$COMPILER_TARGET_FLAG" "$SCRIPT_DIR/main.c" 2>/dev/null; then
    echo "FAIL: ccompiler rejected main.c" >&2
    exit 1
fi

for base in shapes main; do
    if ! nasm -f "$NASM_FMT" "${base}.asm" -o "${base}.o" 2>/dev/null; then
        echo "FAIL: nasm failed for ${base}" >&2
        exit 1
    fi
done

if ! "${LINK_CMD[@]}" shapes.o main.o -o test_compound_literal_multifile 2>/dev/null; then
    echo "FAIL: link failed" >&2
    exit 1
fi

./test_compound_literal_multifile
actual=$?
if [ "$actual" -ne 0 ]; then
    echo "FAIL: expected exit 0, got $actual" >&2
    exit 1
fi

exit 0
