# stage-115 Transcript: Constant Expressions in Array Dimension Bounds

## Summary

Stage 115 extends the array-dimension parser to accept full C99 integer constant expressions — arithmetic, bitwise, shift, relational, equality, logical AND/OR, ternary, `sizeof`, parentheses, enum constants, and macro-expanded identifiers — instead of requiring bare integer literals. Previously, patterns like `int a[LIMIT + 1]` where `LIMIT` is a macro were rejected with "expected ']', got '+'". After this stage, all four array-dimension sites in the parser call the existing `eval_const_expr` function to evaluate the complete expression at parse time. The change is parser-only: no new tokens, no AST modifications, no codegen changes. The evaluator has been available since stage 99 and is battle-tested across self-hosting cycles.

## Changes Made

### Step 1: Parser — Site A (parse_type_name bracket loop)

- Modified `parse_type_name` bracket loop (~line 1122) used for `sizeof(int[N+1])`, compound-literal type-names, and `va_arg` type-names.
- Replaced literal-only read block (`if (current != INT_LITERAL)` error, then consume and store) with a call to `eval_const_expr(parser, "array dimension")`.
- Preserved empty-dimension special case: when token after `[` is `]`, continue to next iteration without calling the evaluator.
- Preserved zero-check validation after evaluation: "array size must be greater than zero".

### Step 2: Parser — Site B (parse_direct_declarator parenthesized form)

- Modified `parse_direct_declarator` parenthesized form (~line 1207) used for `int (*a)[N]` and similar parenthesized-star declarators.
- Replaced conditional block (`if INT_LITERAL { ... } else if != RBRACKET { error }`) with a single guard: `if (current != RBRACKET) { eval_const_expr(...); ... }`.
- Preserves empty-bracket case naturally: when token is `]`, evaluator is not called and `has_size` remains unset.
- Preserved zero-check validation.

### Step 3: Parser — Sites C and D (parse_direct_declarator non-paren)

- Modified non-paren first-dimension block (~line 1288) used for `int a[N]`, `typedef int Row[N]`, and struct member arrays.
- Replaced literal-only conditional with `if (current != RBRACKET) { eval_const_expr(...); ... }`.
- Preserved empty-first-dimension case (`int a[]`): no evaluator call when bracket is empty.
- Preserved zero-check validation.
- Modified non-paren additional-dimensions block (~line 1309) used for second and subsequent dimensions in `int a[N][M]`.
- Replaced literal-only block (no conditional, just error if not `INT_LITERAL`) with direct `eval_const_expr()` call.
- Additional dimensions may never be empty in C99, so no special-case guard needed here.
- Preserved zero-check validation.

### Step 4: Version

- Bumped `VERSION_STAGE` in `src/version.c` to `"01150000"`.

### Step 5: Tests

- Added 7 valid test files in appropriate category subdirectories:
  - `test/valid/declarations/test_global_array_dim_macro_plus_1__42.c` — macro addition at global scope
  - `test/valid/declarations/test_global_array_dim_multiply__42.c` — macro multiplication at global scope
  - `test/valid/declarations/test_typedef_array_dim_const_expr__42.c` — const-expr in typedef array bound
  - `test/valid/arrays/test_local_array_dim_const_expr__42.c` — const-expr in local array declaration
  - `test/valid/arrays/test_multi_array_dim_const_expr__42.c` — const-expr in multidimensional array
  - `test/valid/structs/test_struct_member_array_dim_const_expr__42.c` — const-expr in struct member array
  - `test/valid/expressions/test_sizeof_array_dim_const_expr__20.c` — const-expr in sizeof type-name
- Added 2 invalid test files:
  - `test/invalid/arrays/test_array_dim_runtime_var__is_not_an_integer_constant_expression.c` — runtime variable rejected
  - `test/invalid/arrays/test_array_dim_negative_const_expr__array_size_must_be_greater_than_zero.c` — negative const-expr rejected
- Renamed 3 existing invalid tests to match new error messages:
  - Previous message: "array size must be an integer literal"
  - New messages: "is not an integer constant expression" (runtime vars) or "array size must be greater than zero" (negative values)

### Step 6: Self-hosting

- Executed `./build.sh --mode=self-host` (C0→C1→C2 cycle).
- C0 (GCC-built): `00.02.01150000.00906` — all 1850 tests pass.
- C1 (C0-bootstrapped): `00.02.01150000.00907` — all 1850 tests pass.
- C2 (C1-bootstrapped): `00.02.01150000.00908` — all 1850 tests pass.
- No source changes needed during bootstrap; compiler uses only literal constants in array dimensions.
- Updated `docs/self-compilation-report.md` with stage-115 results.

## Final Results

All 1850 tests pass at C0, C1, and C2 (1168 valid, 258 invalid, 88 integration, 50 print-AST, 100 print-tokens, 21 print-asm, 165 unit). No regressions. Self-hosting cycle completes successfully with no source modifications required.

## Session Flow

1. Read stage spec and kickoff document to understand four parser sites and test expectations.
2. Reviewed existing `eval_const_expr` implementation in `src/parser.c` (available since stage 99).
3. Applied parser fixes at four sites, replacing literal-only reads with evaluator calls.
4. Bumped version in `src/version.c` to `"01150000"`.
5. Built compiler and ran `test/valid/run_tests.sh` to verify existing tests still pass.
6. Created 7 valid test files covering global arrays, local arrays, multidim arrays, typedefs, struct members, and sizeof type-names.
7. Created 2 invalid test files for runtime-variable rejection and negative-value rejection.
8. Renamed 3 existing invalid test files to reflect updated error messages from evaluator.
9. Ran full test suite `./test/run_all_tests.sh` — all 1850 tests pass.
10. Executed self-hosting cycle `./build.sh --mode=self-host` — C0→C1→C2 all pass with no compiler modifications.
11. Updated `docs/self-compilation-report.md` with stage-115 checkpoint results.

## Notes

Three invalid test files were renamed to match the actual error messages emitted by `eval_const_expr`:

- When a runtime variable like `int n` appears in an array dimension, `eval_const_expr` rejects it with "is not an integer constant expression" (since VLAs are not supported).
- When a negative constant expression like `NEG + 0` (where `NEG = -1`) is evaluated, the zero-check validation rejects it with "array size must be greater than zero".
- These changes are semantically correct: the parser now accurately reports why an array dimension is invalid.

The evaluator's grammar and operator precedence remain unchanged from stages 99–104; only the call sites expanded from case-label and enum-constant contexts to include all array-dimension positions. The compiler's own source code uses only literal constants as array dimensions (e.g., `int dims[MAX_ARRAY_DIMS]`), so no bootstrap modifications were needed.
