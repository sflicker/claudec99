# Milestone Summary

## Stage 117: FP Struct Member Read — Complete

stage-117 ships three codegen bug fixes addressing silent data corruption when floating-point (`double` or `float`) struct and union fields are read as rvalues.

- **Codegen — Bug 1 (rvalue FP load)**: `codegen_expression()` AST_MEMBER_ACCESS and AST_ARROW_ACCESS rvalue blocks now emit `movss xmm0, [rax]` / `movsd xmm0, [rax]` for TYPE_FLOAT/TYPE_DOUBLE fields instead of the incorrect `mov eax, [rax]` (4-byte integer). Fixes all three member-access forms: direct dot (`s.x`), arrow (`p->x`), and subscript-then-dot (`arr[i].x`).
- **Codegen — Bug 2 (type inference for FP fields)**: `expr_result_type()` AST_MEMBER_ACCESS (VAR_REF and DEREF bases) and AST_ARROW_ACCESS cases now check `type_is_fp(f->kind)` and return the field's FP type instead of falling through to `promote_kind(TYPE_DOUBLE)` which returns TYPE_INT. Ensures binary operators on struct FP fields use the SSE2 code path.
- **Codegen — Bug 3 (subscript-then-member type inference)**: `expr_result_type()` and `sizeof_type_of_expr()` AST_MEMBER_ACCESS cases now handle AST_ARRAY_INDEX bases (`bodies[j].x`) by walking to the array variable, resolving its element type, finding the named field, and returning the correct FP/pointer/int kind. Also adds FP-aware `else if (type_is_fp)` branches to existing base cases in `sizeof_type_of_expr()` for consistency.
- **Tests**: 6 new valid tests added covering local struct FP arithmetic (dot), pointer-to-struct FP arithmetic (arrow), global struct array FP field arithmetic via subscript-then-dot, struct array FP field updates, float field arithmetic, and FP field subtraction patterns.
- **Version**: Bumped to `"01170000"` in `src/version.c`.
- **Self-host**: C0→C1→C2 passes cleanly; all 1863 tests pass at each stage. No source changes needed during bootstrap (compiler uses local double variables, not struct FP fields, for its own arithmetic).
- **Notes**: Writing to FP struct fields already worked correctly (assignment path had proper guards); only the read (rvalue) path was broken. All 1863 tests pass (1181 valid, 258 invalid, 88 integration, 50 print-AST, 100 print-tokens, 21 print-asm; 165 unit).
