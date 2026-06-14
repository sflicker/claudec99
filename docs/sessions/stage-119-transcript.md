# stage-119 Transcript: FP Compound Assignment on Struct Members

## Summary

Stage 119 fixes floating-point compound assignment (`+=`, `-=`, `*=`, `/=`) on struct fields when both operands are members of global (file-scope) struct variables. The root cause was a missing `codegen_find_global` fallback in `expr_result_type()` and `sizeof_type_of_expr()` functions. When `codegen_find_var` returned NULL for a global struct base, these functions fell back to TYPE_INT instead of querying the global-variable table. This silent fallback caused the floating-point arithmetic path to be skipped entirely, leaving integer instructions operating on IEEE 754 bit patterns, producing nonsensical exit codes.

The fix required six targeted changes in `src/codegen.c` to add global-variable lookup fallbacks in the VAR_REF and DEREF branches of member-access type-resolution, plus one additional fix to `emit_arrow_addr()` that was discovered during testing. A sixth bug in the same function prevented `gp->field` from working as an lvalue when `gp` was a global pointer-to-struct variable. No tokenizer, parser, or AST changes were needed — this is a pure codegen stage.

## Changes Made

### Step 1: Codegen — Fix expr_result_type() Bug 1 (VAR_REF base)

- Modified `expr_result_type()` in `src/codegen.c`, `AST_MEMBER_ACCESS` case, VAR_REF branch (~line 2096).
- Factored out `struct_type` variable to hold either local or global struct base.
- Added fallback to `codegen_find_global()` when `codegen_find_var()` returns NULL, checking for global struct/union types.
- Enables correct type reporting for `g.x` where `g` is a file-scope struct variable.

### Step 2: Codegen — Fix expr_result_type() Bug 2 (DEREF base)

- Modified `expr_result_type()` in `src/codegen.c`, `AST_MEMBER_ACCESS` case, DEREF branch (~line 2118).
- Factored out `ptr_base` variable to hold either local or global pointer-to-struct base.
- Added fallback to `codegen_find_global()` when local lookup fails.
- Enables correct type reporting for `(*gp).x` where `gp` is a file-scope pointer-to-struct.

### Step 3: Codegen — Fix expr_result_type() Bug 3 (AST_ARROW_ACCESS)

- Modified `expr_result_type()` in `src/codegen.c`, `AST_ARROW_ACCESS` case (~line 2177).
- Factored out `ptr_base` variable to hold either local or global pointer-to-struct base.
- Added fallback to `codegen_find_global()` when local lookup fails.
- Enables correct type reporting for `gp->x` where `gp` is a file-scope pointer-to-struct.

### Step 4: Codegen — Fix sizeof_type_of_expr() Bug 4 (VAR_REF base)

- Modified `sizeof_type_of_expr()` in `src/codegen.c`, `AST_MEMBER_ACCESS` case (~line 1856).
- Factored out `struct_type` variable to hold either local or global struct base.
- Added fallback to `codegen_find_global()` when local lookup fails.
- Ensures `sizeof(g.x)` returns correct size (8 for double) instead of 4.

### Step 5: Codegen — Fix sizeof_type_of_expr() Bug 5 (AST_ARROW_ACCESS)

- Modified `sizeof_type_of_expr()` in `src/codegen.c`, `AST_ARROW_ACCESS` case (~line 1910).
- Factored out `ptr_base` variable to hold either local or global pointer-to-struct base.
- Added fallback to `codegen_find_global()` when local lookup fails.
- Ensures `sizeof(gp->x)` returns correct size for global pointer-to-struct fields.

### Step 6: Codegen — Fix emit_arrow_addr() Bug 6 (VAR_REF base)

- Modified `emit_arrow_addr()` in `src/codegen.c`, VAR_REF base handling.
- Added fallback to `codegen_find_global()` when `codegen_find_var()` returns NULL.
- Enables `gp->field` to work correctly as an lvalue when `gp` is a file-scope pointer-to-struct.
- This bug was discovered during test case `test_global_ptr_struct_fp_add_assign__9.c` which requires lvalue support for global pointer-to-struct member access.

### Step 7: Tests — Add seven new valid test cases

- Created `test/valid/structs/test_global_struct_fp_add_assign__7.c`: global struct with FP `+=`, exits 7.
- Created `test/valid/structs/test_global_struct_fp_sub_assign__2.c`: global struct with FP `-=`, exits 2.
- Created `test/valid/structs/test_global_struct_fp_mul_assign__6.c`: global struct with FP `*=`, exits 6.
- Created `test/valid/structs/test_global_ptr_struct_fp_add_assign__9.c`: global pointer-to-struct with FP `->` compound assign, exits 9.
- Created `test/valid/structs/test_local_struct_fp_add_assign__5.c`: local struct regression test, exits 5.
- Created `test/valid/structs/test_mixed_struct_fp_assign__10.c`: mixed local and global struct FP fields, exits 10.
- Created `test/valid/structs/test_global_struct_fp_accumulate__15.c`: global struct FP accumulator loop, exits 15.

### Step 8: Version — Bump stage number

- Updated `src/version.c`: bumped `VERSION_STAGE` from "01180000" to "01190000".

## Final Results

All 1879 tests pass (1195 valid, 260 invalid, 88 integration, 50 print-AST, 100 print-tokens, 21 print-asm, 165 unit). Previous total: 1872 (7 new valid tests added).

Self-host verification: C0→C1→C2 all three passes produced identical binaries with no source changes during bootstrap. All 1879 tests pass at every stage.

No regressions; the stage is a pure codegen fix with no grammar or parser changes.

## Session Flow

1. Read spec and supporting documentation
2. Analyzed root-cause chain: missing global-variable fallback in `expr_result_type()` and `sizeof_type_of_expr()`
3. Planned six fixes across type-resolution functions
4. Implemented Bug 1 fix: `expr_result_type()` VAR_REF branch global fallback
5. Implemented Bug 2 fix: `expr_result_type()` DEREF branch global fallback
6. Implemented Bug 3 fix: `expr_result_type()` AST_ARROW_ACCESS global fallback
7. Implemented Bug 4 fix: `sizeof_type_of_expr()` VAR_REF branch global fallback
8. Implemented Bug 5 fix: `sizeof_type_of_expr()` AST_ARROW_ACCESS global fallback
9. Built and ran motivating test case (`g.x=3.0; g.y=4.0; g.x+=g.y; return (int)g.x;` → exits 7)
10. Discovered Bug 6 during test case `test_global_ptr_struct_fp_add_assign__9.c` requiring lvalue support
11. Implemented Bug 6 fix: `emit_arrow_addr()` VAR_REF base global fallback
12. Added seven new valid test cases to `test/valid/structs/`
13. Ran full test suite (`./test/run_all_tests.sh`) — all 1879 tests pass
14. Bumped version to stage 119
15. Self-hosted C0→C1→C2 cycle — all tests pass with no bootstrap defects
16. Updated `docs/self-compilation-report.md` with stage-119 results

## Notes

**Root cause pattern**: The compiler's type-resolution functions (`expr_result_type()` and `sizeof_type_of_expr()`) were split between local and global lookups but lacked fallback from one to the other. Functions like `codegen_find_var()` search only the local-variable stack; globals live in a separate table (`codegen_find_global()`). When a global struct was accessed in a type-resolution context, the local lookup returned NULL and the code silently fell back to TYPE_INT instead of trying the global lookup.

**Why the bug was hidden**: The bug only manifested when **both** operands of a compound-assign operation were members of the same global struct. If one operand was a scalar literal (`g.x += 1.0`), the literal's known FP type forced the correct arithmetic path. If the local struct variant was used (`v.x += v.y`), the local lookup succeeded. Only the global-struct-to-global-struct case failed.

**Bootstrap impact**: The compiler's own source does not use file-scope structs with FP fields in arithmetic expressions. All floating-point computation is via local `double` variables. The bootstrap cycle proceeded cleanly with no changes to the compiler's own source.

**Scope limitation**: The DEREF branch of `sizeof_type_of_expr()` was intentionally not fixed (`sizeof((*gp).x)` for global pointer-to-struct) because this pattern is essentially never written; it remains a known gap consistent with the stage specification.
