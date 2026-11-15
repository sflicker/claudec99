#!/bin/bash
# Stage 181: ARM64 backend skeleton (--target=arm64-macos), text-only so it
# runs on any host:
#   1. return42.c compiles to return42.s (not .asm) containing the expected
#      ARM64 symbol, immediate load and return.
#   2. An unsupported construct (local variable) is rejected with
#      "arm64-macos: ... not yet supported".
#   3. A literal outside 0..65535 is rejected as out of range.

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/../../.." && pwd)"
# Stage 180: host/target settings (TARGET, NASM_FMT, LINK_CMD, run_timeout).
. "$PROJECT_DIR/test/lib/platform.sh"
COMPILER="$PROJECT_DIR/build/ccompiler"
WORK="$PROJECT_DIR/build/test_tmp_integration/test_arm64_skeleton_asm"

rm -rf "$WORK"
mkdir -p "$WORK"
cd "$WORK" || exit 1

fail() { echo "FAIL: $*" >&2; exit 1; }

# 1. Successful compile: <name>.s, expected instructions.
run_timeout 30 "$COMPILER" --target=arm64-macos "$SCRIPT_DIR/return42.c" 2>/dev/null \
    || fail "compiling return42.c for arm64-macos"
[ -f return42.s ] || fail "return42.s not produced"
[ ! -f return42.asm ] || fail "return42.asm produced for arm64-macos"
# Whitespace-normalize: collapse tabs/spaces to single spaces, trim.
norm=$(sed -e 's/[[:space:]][[:space:]]*/ /g' -e 's/^ //' -e 's/ $//' return42.s)
for want in ".globl _main" "_main:" "mov w0, #42" "ret"; do
    printf '%s\n' "$norm" | grep -qxF "$want" || fail "missing '$want' in return42.s"
done

# 2. Unsupported construct.
err=$(run_timeout 30 "$COMPILER" --target=arm64-macos "$SCRIPT_DIR/local_var.c" 2>&1 >/dev/null)
[ $? -ne 0 ] || fail "local_var.c should be rejected"
case "$err" in
    *"arm64-macos:"*"not yet supported"*) ;;
    *) fail "unexpected local_var.c error: $err" ;;
esac

# 3. Out-of-range literal.
err=$(run_timeout 30 "$COMPILER" --target=arm64-macos "$SCRIPT_DIR/big_literal.c" 2>&1 >/dev/null)
[ $? -ne 0 ] || fail "big_literal.c should be rejected"
case "$err" in
    *"arm64-macos: integer literal 70000 out of range"*) ;;
    *) fail "unexpected big_literal.c error: $err" ;;
esac

exit 0
