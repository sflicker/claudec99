# stage-114 Transcript: Fix Issues with Legacy Valid Tests

## Summary

Stage 114 fixed 24 failing tests that were imported from a legacy project into test/valid/legacy_project/. The failures fell into four categories: 3 test files with exit codes outside the 0–255 range (renamed to valid unsigned 8-bit equivalents), 1 test file with an invalid array initializer (modified by removing the extra element), 2 missing compiler features (nested brace array initialization and string literal subscripting), and 6 compiler bugs (FP array subscript read/write, expr_result_type type inference, mixed FP/int ternary operator, and nested brace initialization of multidimensional local/global arrays).

After fixes were complete, all 219 tests from the legacy project were migrated from test/valid/legacy_project/ to appropriate category subfolders within the permanent test structure (191 new unique tests added to 13 categories; 28 duplicates merged with existing tests). The legacy_project directory was then deleted.

The stage brings the total test count to 1841 (1161 valid, 256 invalid, 88 integration, 50 print-AST, 100 print-tokens, 21 print-asm; 165 unit). All tests pass. The self-host C0→C1→C2 bootstrap cycle completes successfully with no source changes needed.

## Changes Made

### Step 1: Test File Fixes — Exit Code Overflow

- **test_unary_neg__-42.c** → renamed to `test_unary_neg__214.c` (exit code -42 wraps to unsigned 8-bit 214).
- **test_trunc_toward_zero__-3.c** → renamed to `test_trunc_toward_zero__253.c` (exit code -3 wraps to unsigned 8-bit 253).
- **test_char_promotion__260.c** → renamed to `test_char_promotion__4.c` (exit code 260 mod 256 = 4).

### Step 2: Test File Fixes — Invalid Initializer

- **test_long_array__42.c** → modified to remove the 11th element from array with 10-element declaration, fixing the initializer count mismatch.

### Step 3: Code Generator — FP Array Subscript WRITE

- Fixed `AST_ARRAY_INDEX` write path in `emit_store_to_address`.
- Previously emitted `mov [rbx], rax` (truncating IEEE 754 double bits).
- Now emits `movss [rbx], xmm0` for float elements and `movsd [rbx], xmm0` for double elements.
- Result value remains in the appropriate FP register (xmm0 for float/double).

### Step 4: Code Generator — FP Array Subscript READ

- Fixed `AST_ARRAY_INDEX` read path in `emit_expr`.
- Previously emitted `mov rax, [rax]` followed by `movsxd rax, eax`, silently truncating double values.
- Now emits `movss xmm0, [rax]` for float elements and `movsd xmm0, [rax]` for double elements.
- Result value is loaded directly into the appropriate FP register.

### Step 5: Code Generator — expr_result_type for Array Subscripts

- Fixed pre-evaluation type inference for `AST_ARRAY_INDEX` case.
- Previously returned `TYPE_LONG` for double array elements via `promote_kind(type_kind_from_size(8))`.
- Now returns `element->kind` directly for FP element types, preserving float/double distinction.
- Ensures downstream FP binary operations use SSE paths instead of integer paths.

### Step 6: Code Generator — Nested Brace Local Array Init

- Added `emit_local_array_init()` helper function for recursive handling of `AST_INITIALIZER_LIST` elements.
- Supports multidimensional local arrays like `int A[2][3] = {{36,1,0},{2,3,0}}`.
- Recursively processes nested brace elements and emits stack initialization code for each sub-list.
- Previously, nested brace values were silently discarded.

### Step 7: Code Generator — Nested Brace Global Array Init

- Added `emit_global_array_elements()` helper function for global array initialization with sub-brace grouping.
- Supports file-scope multidimensional arrays like `int A[2][2] = {{36,1},{2,3}}`.
- Recursively processes nested initializer lists and emits data section values.

### Step 8: Code Generator — Mixed FP/int Ternary

- Fixed `AST_CONDITIONAL_EXPR` codegen for branches with different numeric types.
- Pre-computes both branch result types via `expr_result_type`.
- Finds the common FP type (float or double) when one branch is FP and the other is integer.
- Calls `emit_fp_widen_if_needed()` after evaluating each branch so both converge to the same canonical register/type before the merge point.
- Enables patterns like `(condition) ? float_val : int_val` to type-convert correctly.

### Step 9: Parser — String Literal Subscript Base

- Added `AST_STRING_LITERAL` to the list of allowed subscript base expressions in `parse_postfix_expr`.
- Previously only identifiers (converted to pointers) and other lvalue/pointer expressions were allowed.
- Now enables `"abc"[2]` to parse correctly.

### Step 10: Code Generator — String Literal Subscript

- Added `AST_STRING_LITERAL` case in `emit_array_index_addr()`.
- Emits the string literal address to `rax` via `emit_addr_of_string_literal()`.
- Performs standard pointer arithmetic (rax + index * element_size) to compute the subscript address.
- Works seamlessly with the existing array subscript read/write paths.

### Step 11: Tests — Legacy Migration

- Identified target category for each of 219 legacy tests based on test name and function.
- Migrated 191 unique tests to category subfolders: arithmetic (12), arrays (23), bitwise (6), casting (5), comparisons (10), control_flow (18), declarations (11), expressions (13), floating_point (12), functions (15), misc (12), pointers (14), strings (8).
- Merged 28 duplicate tests with existing tests in target categories (content comparison and manual consolidation).
- Deleted empty test/valid/legacy_project/ directory.

### Step 12: Tests — Verification

- Ran full test suite: all 1841 tests pass (1161 valid, 256 invalid, 88 integration, 50 print-AST, 100 print-tokens, 21 print-asm; 165 unit).
- No regressions.
- Bootstrap cycle C0→C1→C2 verified: all 1841 pass at each stage, no compiler source changes needed.

## Final Results

All 1841 tests pass (1161 valid, 256 invalid, 88 integration, 50 print-AST, 100 print-tokens, 21 print-asm; 165 unit). No regressions. Self-host C0→C1→C2 cycle verified with no source changes needed during bootstrap.

## Session Flow

1. Read stage 114 spec and legacy project failure report.
2. Fixed 4 test file issues (3 renamed, 1 modified).
3. Implemented 7 compiler fixes in src/codegen.c and src/parser.c.
4. Ran test suite to confirm all 24 legacy tests now pass.
5. Migrated 219 legacy tests to appropriate category subfolders.
6. Verified all 1841 tests pass in new structure.
7. Performed self-host C0→C1→C2 cycle with full test suite.
8. Deleted legacy_project directory and updated VERSION_STAGE.
9. Generated milestone and transcript documentation.
10. Updated README.md with new test totals and stage 114 capability entry.

## Implementation Notes

### FP Array Subscript Bugs

The array subscript codegen path had parallel bugs in both read and write directions. The write path was using `mov [addr], rax` (integer instruction) to store values that were in xmm0/xmm1 (FP registers), causing silent data corruption. The read path was loading double values via `mov rax, [rax]` then `movsxd rax, eax`, which truncated the double mantissa to a 32-bit integer pattern. Both were fixed by properly branching on element type and using the correct SSE instructions (`movss`/`movsd`).

### Type Inference Bug

The `expr_result_type` function, used for pre-evaluating expression types before codegen, was promoting all 8-byte scalar types to `TYPE_LONG` via `promote_kind(type_kind_from_size(8))`. This caused FP binary operations on array elements to incorrectly dispatch to integer code paths. The fix simply returns `element->kind` directly for FP element types, preserving the type distinction.

### Ternary Operator with Mixed FP/int

The ternary operator codegen was not handling type widening when branches produced different numeric types. If the true branch evaluated to a double and the false branch to an int, the int value was left in `rax` with no conversion. The fix pre-computes both branch types, identifies when one is FP, and applies the standard FP widening logic (`emit_fp_widen_if_needed`) after each branch evaluation, ensuring both paths converge to the same type before the merge point.

### Nested Brace Initialization

ClaudeC99 already had basic array initialization support, but the codegen path for local and global arrays did not recursively handle `AST_INITIALIZER_LIST` elements. When a user wrote `int A[2][3] = {{1,2,3},{4,5,6}}`, the outer brace list was processed but the inner brace lists (which should populate sub-arrays) were treated as a flat sequence and values were often lost. Two new helper functions (`emit_local_array_init` and `emit_global_array_elements`) fix this by recursively descending through nested initializer lists and properly indexing into multidimensional array structures.

### Legacy Test Migration

The 219 legacy tests were manually categorized and moved to permanent category subfolders. Twenty-eight tests already existed in their target categories; these were kept and consolidated by ensuring the test is present (duplicates discarded). This avoids redundant testing while preserving test coverage. The migration was completed before deleting the legacy_project directory to ensure no loss of test data.
