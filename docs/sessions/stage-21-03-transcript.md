# stage-21-03 Transcript: Function Declaration Compatibility Checks

## Summary

Stage 21-03 extends function declaration compatibility checking to validate return types and parameter types across repeated declarations and definitions. Previously, the compiler (from stage 21-02) only checked for parameter count mismatch and duplicate definitions. This stage adds two new semantic checks: if a function is re-declared or defined with a different return type than its first declaration, the compiler now emits `error: function '<name>' return type mismatch`; similarly, if any parameter's type differs between declarations, it emits `error: function '<name>' parameter type mismatch at position <n>`.

The implementation leverages the `FuncSig` struct introduced in stage 21-02, which already captured `return_type` and `param_types[]`. The stage adds the missing comparison logic to detect and report type mismatches during function registration.

## Changes Made

### Step 1: Parser — Return Type and Parameter Type Mismatch Checks

- Added return type comparison logic in `parser_register_function` (src/parser.c): when a function signature is encountered that matches an existing function by name, the return types are compared; if they differ, an error is emitted.
- Added parameter type comparison logic: each parameter type in the new signature is compared against the corresponding position in the existing function's parameter types; if any mismatch is found, an error is emitted with the parameter position.
- Both checks occur after parameter count validation (already present from earlier stages).

### Step 2: Tests

- Created `test/invalid/test_invalid_107__return_type_mismatch.c`: declares a function with return type `int`, then defines it with return type `long`; compiler rejects with `error: function 'f' return type mismatch`.
- Created `test/invalid/test_invalid_108__parameter_type_mismatch.c`: declares a function with a `char` parameter, then defines it with an `int` parameter; compiler rejects with `error: function 'f' parameter type mismatch at position 0`.
- Valid tests from the spec (repeating matching prototype, prototype before definition, pointer return/param) were already covered by existing test cases from prior stages (e.g., stage 21-02 tests); no new valid tests were required.

### Step 3: Documentation

- Updated README.md to reflect stage 21-03 completion and new test totals (394 valid, 110 invalid, plus print suites).
- No grammar changes needed; the language syntax and grammar remain unchanged.

## Final Results

All 504 tests pass: 394 valid, 110 invalid, 24 print-AST, 88 print-tokens, 19 print-asm. No regressions. The compiler now rejects programs with function return type or parameter type mismatches as required by the spec.

## Session Flow

1. Read spec and template requirements
2. Reviewed milestone and transcript format guidelines
3. Examined README.md to identify lines needing update
4. Noted summary of completed implementation (return type and parameter type checks in parser_register_function)
5. Identified test changes: 2 new invalid tests (return type mismatch, parameter type mismatch)
6. Verified all existing valid tests from spec were already covered
7. Generated milestone summary with subsystem focus
8. Generated transcript with structured changes and results
9. Updated README.md with stage 21-03 reference and test totals
