# Stage 114 — Fix Issues with Legacy Valid Tests

## Spec Summary

Fix 24 failing tests imported from a legacy project (in `test/valid/legacy_project/`). The fixes span test file issues, missing compiler features, and compiler bugs. After all tests pass, move each test to an appropriate category subfolder and delete the `legacy_project/` folder.

**Test failure categories**:
- 3 test file naming errors (negative exit codes out of unsigned 8-bit range)
- 1 test file coding error (invalid C99)
- 2 missing compiler features (string literal subscript, global nested array initialization)
- 3 compiler bugs (local nested array initialization, FP array subscript I/O, FP ternary mixed branches)
- 15 other compiler bugs (unspecified in spec; see failure report for details)

## Required Tokenizer Changes

None.

## Required Parser Changes

None.

## Required AST Changes

None.

## Required Code Generation Changes

All changes in `src/codegen.c`:

1. **String literal subscript**: Add `AST_STRING_LITERAL` as valid base type in `emit_array_index_addr` (enables `"abc"[2]`).

2. **Global multidimensional array nested brace initialization**: Support `int A[2][2] = {{36,1},{2,3}}` at file scope; currently only flat lists work.

3. **Local multidimensional array nested brace initialization** (bug fix): Add `emit_local_array_init` helper to recursively initialize sub-arrays from brace lists. Handle `elem_type->kind == TYPE_ARRAY && elem->type == AST_INITIALIZER_LIST` in both VAR_DECL and compound literal paths.

4. **FP array subscript read** (bug fix): When element type is float/double in `AST_ARRAY_INDEX` read path, emit `movss`/`movsd` to xmm0 instead of `mov eax, [rax]`.

5. **FP array subscript write** (bug fix): When element type is float/double in assignment path, emit FP store via `movss`/`movsd` to xmm0.

6. **FP array brace initialization** (bug fix): In local array init loops, use `emit_store_fp_local` instead of `emit_store_local` when element type is FP.

7. **Ternary operator with mixed FP/int branches** (bug fix): Pre-compute result type using `sizeof_type_of_expr` before evaluating branches. After each branch, emit FP conversion if common type is FP and branch produced different type.

## Test Plan

### Phase 1: Test File Fixes
1. Rename `test/valid/legacy_project/expressions/test_unary_neg__-42.c` → `test_unary_neg__214.c` (unsigned wrap of -42)
2. Rename `test/valid/legacy_project/floating_point/test_trunc_toward_zero__-3.c` → `test_trunc_toward_zero__253.c` (unsigned wrap of -3)
3. Rename `test/valid/legacy_project/types/test_char_promotion__260.c` → `test_char_promotion__4.c` (signed char wrap: 250→-6, -6+10=4)
4. Edit `test/valid/legacy_project/array/test_long_array__42.c` to remove 11th initializer element

### Phase 2: Compiler Fixes
Implement code generation changes in order:
1. String literal subscript support
2. Global nested array initialization
3. Local nested array initialization helper
4. FP array subscript read
5. FP array subscript write
6. FP array brace initialization
7. Ternary mixed FP/int branches

Run `test/valid/run_tests.sh` after each fix to track progress.

### Phase 3: Test File Migration
After all tests pass:
1. Move each test from `test/valid/legacy_project/` subfolders to appropriate category subfolders (arrays/, floating_point/, expressions/, types/, etc.)
2. Delete empty `legacy_project/` folder

### Phase 4: Verification
Run full test suite to confirm all legacy tests pass in their new locations.

## Implementation Order

1. Apply test file renames (unary_neg, trunc_toward_zero, char_promotion)
2. Edit array init test (remove 11th element)
3. Implement string literal subscript in codegen
4. Implement global nested array initialization
5. Implement local nested array initialization (new helper + handler)
6. Implement FP array subscript read
7. Implement FP array subscript write
8. Implement FP array brace initialization
9. Implement ternary mixed FP/int branches
10. Run full test suite after each fix
11. Migrate test files to category subfolders
12. Delete `legacy_project/` folder
13. Commit with version bump

## Ambiguities & Decisions

**Ambiguity**: Spec lists 9 specific issue types but mentions 24 failing tests total. The failure report in `docs/defects/legacy-project-test-failure-report.md` provides full details for all failures.

**Decision**: Implement fixes in the order listed above (test file fixes first, then compiler fixes in logical dependency order). This allows incremental validation.

**Decision**: For nested array initialization (both global and local), treat each element of the outer array as a potential initializer list that may be a brace list. Recursive structure mirrors the array type.

**Decision**: FP type checks use `elem_type->kind == TYPE_FLOAT || elem_type->kind == TYPE_DOUBLE` consistently across all FP codegen fixes.

**Decision**: After all legacy tests pass, migrate them to existing category subfolders (arrays/, floating_point/, expressions/, types/, etc.) following test name prefixes as the signal. If no clear category, place in misc/.

**Version bump**: Yes, increment `src/version.c` (compiler bug fixes warrant a version bump).

## Scope

- In scope: test file renames/edits, code generation fixes for missing features and bugs, test migration
- Out of scope: tokenizer, parser, AST node types, grammar
