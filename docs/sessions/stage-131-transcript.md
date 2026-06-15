# stage-131 Transcript: sizeof Produces Unsigned size_t

## Summary

Stage 131 fixes the `sizeof` operator to produce an unsigned `size_t` result per C99 §6.5.3.4. The compiler previously treated `sizeof` as a signed `long`, causing three expression evaluation bugs: `sizeof(int) > -1` evaluated true (incorrect), `sizeof(char) - 2 > 0` evaluated false (incorrect), and `sizeof(int) < 0` evaluated false (correct but for the wrong reason). The fix is two lines in `src/codegen.c`: set `node->is_unsigned = 1` on both `AST_SIZEOF_TYPE` and `AST_SIZEOF_EXPR` nodes when materializing the sizeof value. The existing usual arithmetic conversion infrastructure (`uac_is_unsigned()`) then correctly applies unsigned comparison and arithmetic rules. No tokenizer, parser, or AST changes were needed.

## Changes Made

### Step 1: Root Cause Analysis

- Identified that `node->is_unsigned` flag was never set on sizeof AST nodes, causing the UAC (usual arithmetic conversion) path to treat sizeof results as signed.
- Located the codegen handling in `src/codegen.c` for `AST_SIZEOF_TYPE` and `AST_SIZEOF_EXPR` cases.
- Confirmed that the UAC infrastructure (`uac_is_unsigned()`, comparison/arithmetic codegen paths) already handles unsigned values correctly once the flag is set.

### Step 2: Codegen Fix

- Modified `src/codegen.c` in the `AST_SIZEOF_TYPE` handler: added `node->is_unsigned = 1;` after the sizeof value is computed.
- Modified `src/codegen.c` in the `AST_SIZEOF_EXPR` handler: added `node->is_unsigned = 1;` after the sizeof value is computed.
- No other changes needed; the existing codegen paths for arithmetic, comparisons, and UAC already respect the `is_unsigned` flag.

### Step 3: Version Update

- Bumped `src/version.c` VERSION_STAGE to `"13100000"`.

### Step 4: Regression Test Addition

- Added `test/valid/expressions/test_sizeof_size_t__6.c` — the spec program that validates all three checks:
  - `sizeof(int) > -1` must evaluate false (unsigned UAC converts -1 to ULONG_MAX, so 4 > ULONG_MAX is false).
  - `sizeof(char) - 2 > 0` must evaluate true (unsigned arithmetic: 1 - 2 wraps to ULONG_MAX-1, which is > 0).
  - `(sizeof(int) < 0) == 0` must evaluate true (unsigned value can never be < 0, so the inner comparison is false, and false == 0 is true).
  - Expected exit code: 6 (from all three checks: 0 + 2 + 4).

### Step 5: Bootstrap Fix

- Added `double strtod(const char *nptr, char **endptr);` declaration to `test/include/stdlib.h`.
- This declaration is required because `src/parser.c` (added in stages 126-130 for FP constant-expression evaluation) calls `strtod`, and the stub header was missing the declaration, causing bootstrap compilation to fail.

## Final Results

All 1935 tests pass (1252 valid, 259 invalid, 88 integration, 50 print-AST, 100 print-tokens, 21 print-asm, 165 unit). No regressions. The regression test validates the fix: it now returns exit code 6 as expected by C99.

Self-host C0→C1→C2 verified: C0 (gcc-built compiler) compiles all sources to produce C1. C1 runs all 1935 tests successfully. C1 compiles all sources to produce C2. C2 runs all 1935 tests successfully. No further source changes needed.

## Session Flow

1. Analyzed spec requirements for sizeof unsigned type and C99 usual arithmetic conversions.
2. Reviewed spec regression program to understand the three checks.
3. Examined codegen paths for sizeof (`AST_SIZEOF_TYPE` and `AST_SIZEOF_EXPR`).
4. Confirmed that UAC infrastructure already supports unsigned operands via the `is_unsigned` flag.
5. Implemented two-line fix: set `node->is_unsigned = 1` in both sizeof handlers.
6. Bumped version number to stage 131.
7. Built C0 compiler.
8. Ran test suite: all tests pass, including the new regression test (exit code 6).
9. Detected bootstrap issue: strtod missing from stub stdlib.h header.
10. Added strtod declaration to test/include/stdlib.h.
11. Re-ran test suite: all 1935 tests pass at C0.
12. Bootstrapped C0 to C1: compiled all sources with C0.
13. Ran test suite on C1: all 1935 tests pass.
14. Bootstrapped C1 to C2: compiled all sources with C1.
15. Ran test suite on C2: all 1935 tests pass.
16. Verified self-host stability: C0, C1, C2 are behavior-identical.

## Notes

The sizeof operator in C99 has type `size_t`, which is an unsigned integer type. The fix leverages the compiler's existing usual arithmetic conversion (UAC) infrastructure, which already correctly handles unsigned operands once the `is_unsigned` flag is set on an AST node. No new operators, instructions, or type-checking logic was needed; the semantic correctness comes entirely from marking sizeof results as unsigned so that comparisons with signed operands (e.g., `sizeof(int) > -1`) and arithmetic operations (e.g., `sizeof(char) - 2`) apply unsigned conversion rules.

The three specific checks in the regression test illuminate the differences:
1. `sizeof(int) > -1`: unsigned UAC converts -1 to ULONG_MAX (largest unsigned), so 4 > ULONG_MAX is false.
2. `sizeof(char) - 2 > 0`: unsigned arithmetic wraps 1 - 2 to a large unsigned value, so the comparison is true.
3. `(sizeof(int) < 0) == 0`: unsigned values are never < 0, so the inner comparison is false, and false == 0 is true.

All three checks add to score 6 when sizeof is correctly unsigned, but would incorrectly yield 5 if sizeof were signed.

The strtod bootstrap fix is unrelated to sizeof correctness; it is a maintenance issue discovered during self-hosting. The `src/parser.c` module uses `strtod` to evaluate FP constant expressions (feature added in stages 126-130), but the stub `test/include/stdlib.h` was not updated to declare it, causing the bootstrap compiler to fail the build step. Adding the declaration resolves the mismatch.
