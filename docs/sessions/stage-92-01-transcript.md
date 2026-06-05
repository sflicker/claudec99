# stage-92 Transcript: Self-Compilation Validation

## Summary

stage 92 validates that the ClaudeC99 compiler achieves full self-compilation. The compiler (C0), when built with gcc, successfully compiles itself to produce C1. All 1306 tests pass with C1. C1 then compiles itself to produce C2, which also passes all 1306 tests. This bootstrapping cycle confirms that the compiler is self-hosting and stable. Six silent bugs in the code generator were discovered and fixed during this process—all related to struct-by-value assignments in various lvalue contexts and sizeof calculations for complex member-access expressions. These fixes ensure correct semantics for the lexer and parser's internal use of struct-by-value parameters and returns.

## Changes Made

### Step 1: Codegen Bug Fixes for Struct-by-Value Assignments

- Fixed `AST_ARRAY_INDEX` LHS assignment path: struct-sized elements now call `emit_struct_addr` + `emit_struct_copy` before the scalar push/pop/switch sequence (previously fell through to `default: mov [rbx], eax`, storing only 4 bytes).
- Fixed `AST_MEMBER_ACCESS` LHS assignment path: applied the same struct/union branch with `emit_struct_addr` + `emit_struct_copy` pattern (dot-access on structs was incorrectly storing only 4 bytes of multi-byte results).
- Fixed `AST_ARROW_ACCESS` LHS assignment path: added struct/union branch to properly handle arrow-access assignment (ptr->member assignment was failing silently).
- Fixed `AST_DEREF` LHS assignment path: added struct/union branch for dereference assignment (`*ptr = struct_func()` was incorrectly truncated).

### Step 2: Codegen Bug Fixes for sizeof

- Fixed `sizeof(ptr->field)` where field is an array/struct/union: added early-return cases in `AST_SIZEOF_EXPR` for `AST_ARROW_ACCESS` when the member has a full_type (was incorrectly returning `sizeof_type_of_expr` which returns TypeKind, not byte count).
- Fixed `sizeof(obj.field)` where field is an array/struct/union: added early-return case in `AST_SIZEOF_EXPR` for `AST_MEMBER_ACCESS` with the same logic.
- Fixed `sizeof(arr[i].field)`: added early-return case in `AST_SIZEOF_EXPR` for `AST_MEMBER_ACCESS` where the base is `AST_ARRAY_INDEX`.

### Step 3: Infrastructure and Constants

- Added `MAX_CALL_LAYOUT_ITEMS 24` to `include/constants.h` (replaces the expression `FUNC_MAX_PARAMS + 8` which the compiler cannot evaluate as an array dimension).
- Added `is_static_linkage` field to `GlobalVar` struct in `include/codegen.h` to track non-static file-scope variables.
- Updated code generator to emit `global` NASM directives for non-static file-scope variables (required for proper linking).

### Step 4: Stub Headers and Expected Output Updates

- Updated `test/include/time.h` to add `struct tm`, `localtime`, and `strftime` declarations (required by libc integration).
- Updated `test/print_asm/test_stage_22_01_global_bss_layout.expected` to include new `global` directives.
- Updated `test/print_asm/test_stage_22_01_global_rip_relative.expected` to include new `global` directives.

### Step 5: Version Update

- Updated `src/version.c`: `VERSION_MINOR` bumped from "01" to "02"; `VERSION_STAGE` set to "00920000" to mark stage 92 completion.

## Final Results

Self-compilation achieved:
- **C0** (gcc-built): Compiles and passes all 1306 tests.
- **C1** (self-compiled by C0): Compiles successfully and passes all 1306 tests.
- **C2** (self-compiled by C1): Compiles successfully and passes all 1306 tests.

No regressions. All test suites report identical results across C0, C1, and C2 builds.

## Session Flow

1. Read spec and identified the self-compilation validation task.
2. Built C0 (gcc compiler) from source.
3. Ran all tests with C0 (baseline: 1306 pass).
4. Compiled the source code with C0 to produce C1.
5. Ran all tests with C1 (confirmed: 1306 pass).
6. Updated version in source (MINOR: 01 → 02; STAGE: 92).
7. Compiled the updated source with C1 to produce C2.
8. Ran all tests with C2 (confirmed: 1306 pass).
9. Documented six codegen bugs discovered during self-compilation and recorded fixes.
10. Updated README.md to reflect completed self-compilation milestone.

## Notes

During self-compilation, the compiler successfully compiled its own lexer and parser, which use struct-by-value `Token` parameters extensively. The bugs discovered were all silent (no compile-time errors) and manifested as incorrect generated assembly or runtime behavior:

- Four bugs involved struct-by-value results being stored at the wrong memory location or with truncated data.
- Two bugs involved sizeof miscalculation for complex member-access expressions.

Each bug was isolated to a specific LHS assignment path or sizeof case and fixed locally. No architectural changes were required; the fixes are surgical and localized to the code generator.

The achievement of C2 stability validates that the compiler architecture is sound and that struct-by-value semantics (added in stage 91-01) are correctly implemented throughout the code generation pipeline.
