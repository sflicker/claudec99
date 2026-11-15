#!/bin/bash
# Stage 181: ARM64 backend skeleton (--target=arm64-macos), text-only so it
# runs on any host:
#   1. return42.c compiles to return42.s (not .asm) containing the expected
#      ARM64 symbol, immediate load and return.
#   2. An unsupported construct (comparison) is rejected with
#      "arm64-macos: ... not yet supported".
#   3. Stage 182: a literal outside 0..65535 compiles and is loaded with
#      movz/movk.

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
err=$(run_timeout 30 "$COMPILER" --target=arm64-macos "$SCRIPT_DIR/comparison.c" 2>&1 >/dev/null)
[ $? -ne 0 ] || fail "comparison.c should be rejected"
case "$err" in
    *"arm64-macos:"*"not yet supported"*) ;;
    *) fail "unexpected comparison.c error: $err" ;;
esac

# 3. Large literal (Stage 182): compiles, loaded with movz + movk.
run_timeout 30 "$COMPILER" --target=arm64-macos "$SCRIPT_DIR/big_literal.c" 2>/dev/null \
    || fail "compiling big_literal.c for arm64-macos"
[ -f big_literal.s ] || fail "big_literal.s not produced"
grep -q "movz" big_literal.s || fail "missing movz in big_literal.s"
grep -q "movk" big_literal.s || fail "missing movk in big_literal.s"

exit 0
