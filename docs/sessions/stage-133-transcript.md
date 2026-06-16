# stage-133 Transcript: Default Argument Promotions In Function Calls

## Summary

Stage 133 implements C99 §6.5.2.2p6 default argument promotions for calls to functions declared without a prototype. The key issue was that `int f();` (empty parameter list) was incorrectly treated as a zero-parameter prototype instead of "no parameter type information." C99 requires that only `int f(void);` is a zero-parameter prototype; calls through no-prototype declarations must apply default argument promotions to all arguments (narrow integer types promoted to int, float promoted to double). The implementation adds two new flags to track no-prototype state and extends call codegen to apply float→double promotion for all arguments when the callee has no prototype.

## Plan

1. Add AST and parser data structures to distinguish no-prototype declarations from zero-parameter prototypes
2. Update parser to detect empty `()` and set the `is_no_prototype` flag
3. Fix `parser_register_function` to allow no-prototype forward declarations and compatible later definitions
4. Update call sites to skip parameter-count validation when the callee has no prototype
5. Extend codegen float→double promotion logic from variadic-only to include no-prototype calls
6. Verify that integer narrow-type promotions work correctly via existing `movsx`/`movzx` mechanisms
7. Add tests and verify self-hosting

## Changes Made

### Step 1: AST and Parser Data Structures

- Added `int is_no_prototype;` field to `ASTNode` in `include/ast.h` to mark function declarations with empty parameter lists
- Added `int has_no_prototype;` field to `FuncSig` in `include/parser.h` to track no-prototype state in the function signature registry
- Both fields default to 0 (prototype with type information, or no declaration)

### Step 2: Parser Detection of Empty Parameter Lists

- In `src/parser.c`, updated the parameter list parsing branch to distinguish `()` (no prototype) from `(void)` (zero-parameter prototype)
- When an opening parenthesis is followed immediately by a closing parenthesis (no `void` keyword), set `node->is_no_prototype = 1`
- When parsing `(void)`, leave `is_no_prototype = 0` (treat as a regular zero-parameter prototype)

### Step 3: Function Registration and Compatibility

- Updated `parser_register_function` in `src/parser.c` to handle no-prototype forward declarations:
  - If a no-prototype declaration is followed by a prototype definition, accept it and allow the definition to proceed
  - If a prototype definition is followed by a no-prototype declaration, accept it
  - Maintain compatibility by allowing both orderings (declaration-first or definition-first)
- Removed the blanket "parameter count mismatch" error when comparing a no-prototype signature (count 0 in registry) with a later definition that has parameters

### Step 4: Call-Site Parameter Validation

- Updated all call-validation sites in `src/codegen.c` to check `sig->has_no_prototype` before enforcing parameter-count matching
- When `has_no_prototype` is set, skip the parameter-count validation since the callee type provides no parameter type information
- Direct calls and indirect calls both respect this flag

### Step 5: Float→Double Promotion in Codegen

- Located the float→double promotion logic in `src/codegen.c` in the direct-call emission path:
  - Phase 1 (memory-class arguments): Extended the condition from `is_variadic` to `is_variadic || node->children[1]->result_is_no_prototype`
  - Phase 2 (register-class arguments): Applied the same extension to XMM argument handling
- Located the float→double promotion logic in the indirect-call path and extended it similarly
- Both phases now emit `cvtss2sd xmm0, xmm0` for `float` arguments when the callee has no prototype

### Step 6: Integer Narrow-Type Promotions

- Verified that integer promotions (char/short → int) work correctly via existing infrastructure:
  - `movsx eax, al` (sign-extend char to int)
  - `movsx eax, ax` (sign-extend short to int)
  - `movzx eax, al` (zero-extend unsigned char to int)
  - `movzx eax, ax` (zero-extend unsigned short to int)
- These load instructions already scale values to register width, so no additional promotion logic was needed

### Step 7: Version Update

- Updated `src/version.c` to stage `13300000` (133 × 100000)

## Final Results

All 1939 tests pass (1257 valid, 258 invalid, 88 integration, 50 print-AST, 100 print-tokens, 21 print-asm; 165 unit). No regressions detected.

Two new tests added:
- `test/valid/varargs/test_default_argument_promotions__127.c` — 7-condition variadic test (char, signed char, unsigned char, short, unsigned short, float, double), expected exit 127. Previously passed; continues to pass.
- `test/valid/functions/test_default_promotions_no_prototype__31.c` — 5-condition no-prototype test (char, short, unsigned short, float, double), expected exit 31. Previously rejected with "parameter count mismatch (0 vs 5)"; now passes.

Self-host verification: C0→C1→C2 cycle passes with all 1939 tests at every stage. The compiler's own source never uses no-prototype declarations, so `is_no_prototype` remains 0 during bootstrap.

## Session Flow

1. Read stage 133 spec and supporting materials
2. Analyzed root cause: C99 §6.7.5.3p14 requires distinguishing `()` (no prototype) from `(void)` (zero-parameter prototype)
3. Reviewed AST and parser data structures in `include/ast.h` and `include/parser.h`
4. Added `is_no_prototype` to AST and `has_no_prototype` to FuncSig for state tracking
5. Updated parser parameter list detection to distinguish empty `()` from `(void)`
6. Fixed `parser_register_function` to allow no-prototype declarations with later prototype definitions
7. Updated all call-validation sites to skip parameter-count checks for no-prototype callees
8. Extended float→double promotion logic in direct and indirect call paths
9. Verified integer promotions work via existing `movsx`/`movzx` infrastructure
10. Updated version to stage 133
11. Built with GCC and ran full test suite: 1939 tests pass
12. Verified self-host C0→C1→C2: all 1939 tests pass at every stage
13. Confirmed no source changes needed during bootstrap

## Notes

**Key insight**: Integer narrow-type promotions (char/short → int) are automatically correct because the load instructions (`movsx`/`movzx`) already sign/zero-extend values to register width. Only `float → double` requires an explicit `cvtss2sd` instruction in the call codegen.

**Design decision**: The `is_no_prototype` and `has_no_prototype` flags are separate from the existing `is_variadic` flag, allowing the compiler to distinguish three function types:
- Prototype with zero parameters: `int f(void);` — `is_variadic=0, has_no_prototype=0`
- No prototype information: `int f();` — `is_no_prototype=1, has_no_prototype=1`
- Variadic prototype: `int f(int, ...);` — `is_variadic=1, has_no_prototype=0`

The float→double promotion logic now triggers when `is_variadic || is_no_prototype`, covering both cases correctly.
