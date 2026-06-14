# stage-117 Transcript: FP Struct Member Read

## Summary

Stage 117 fixes three silent codegen bugs that corrupt data when floating-point (`double` or `float`) struct and union fields are read as rvalues. Bug 1 causes the wrong load instruction to be emitted — `mov eax, [rax]` (4-byte integer) instead of `movsd xmm0, [rax]` (8-byte double) or `movss xmm0, [rax]` (4-byte float) — for all member-access forms (direct dot, arrow, subscript-then-dot). Bug 2 causes the type-inference function `expr_result_type()` to report TYPE_INT instead of the field's actual FP type, making binary arithmetic on struct FP fields incorrectly use the integer code path. Bug 3 causes `expr_result_type()` and `sizeof_type_of_expr()` to fall through and return TYPE_INT when the base of a member access is an array subscript (`arr[i].field`), regardless of the field's type. All three bugs are confined to the code generator; no tokenizer, parser, or AST changes are needed. After these fixes, all patterns of FP struct field access — local structs, pointers to structs, and global struct arrays with subscript-then-member — work correctly, producing exit codes that match GCC.

## Changes Made

### Step 1: Code Generator — Bug 1a: Fix AST_MEMBER_ACCESS rvalue FP load

- Located the AST_MEMBER_ACCESS rvalue block in `codegen_expression()` (~line 3015).
- Identified that `TYPE_FLOAT` and `TYPE_DOUBLE` fields fall through to the `default: sz = 4` case, emitting `mov eax, [rax]` for 8-byte doubles.
- Added FP early-return block immediately before the `int sz = type_size(...)` calculation:
  ```c
  if (f->kind == TYPE_FLOAT) {
      fprintf(cg->output, "    movss xmm0, [rax]\n");
      node->result_type = TYPE_FLOAT;
      return;
  }
  if (f->kind == TYPE_DOUBLE) {
      fprintf(cg->output, "    movsd xmm0, [rax]\n");
      node->result_type = TYPE_DOUBLE;
      return;
  }
  ```
- This pattern mirrors the existing array-member decay branch and ensures FP values land in `xmm0`.

### Step 2: Code Generator — Bug 1b: Fix AST_ARROW_ACCESS rvalue FP load

- Located the AST_ARROW_ACCESS rvalue block in `codegen_expression()` (~line 3049).
- Applied identical FP early-return block immediately before the `int sz = type_size(...)` line.
- Ensures pointer-to-struct FP field reads emit the correct load instructions.

### Step 3: Code Generator — Bug 2a: Fix expr_result_type() AST_MEMBER_ACCESS VAR_REF base

- Located `expr_result_type()` AST_MEMBER_ACCESS case (~line 2072).
- Found the VAR_REF base handling that assigns `t = promote_kind(f->kind)`, which maps TYPE_DOUBLE to TYPE_INT.
- Inserted `else if (type_is_fp(f->kind)) { t = f->kind; }` before the existing `else { t = promote_kind(...) }` block.
- Now FP struct fields report their actual type instead of TYPE_INT, causing binary operators to use the SSE2 code path.

### Step 4: Code Generator — Bug 2b: Fix expr_result_type() AST_MEMBER_ACCESS DEREF base

- Located the parallel DEREF base handling in the same `AST_MEMBER_ACCESS` case (~line 2093).
- Applied identical `else if (type_is_fp(f->kind)) { t = f->kind; }` fix.

### Step 5: Code Generator — Bug 2c: Fix expr_result_type() AST_ARROW_ACCESS base

- Located `expr_result_type()` AST_ARROW_ACCESS case (~line 2117).
- Applied same FP-aware `else if (type_is_fp(f->kind))` branch for consistency.

### Step 6: Code Generator — Bug 3a: Add AST_ARRAY_INDEX base to expr_result_type()

- Identified that `expr_result_type()` AST_MEMBER_ACCESS case had no handler for AST_ARRAY_INDEX bases (`bodies[j].x`).
- Added new conditional block after the two existing base cases and before the final `break`:
  ```c
  if (base_node->type == AST_ARRAY_INDEX && base_node->child_count > 0) {
      ASTNode *arr_base = base_node->children[0];
      Type *elem_type = NULL;
      if (arr_base->type == AST_VAR_REF) {
          LocalVar *lv = codegen_find_var(cg, arr_base->value);
          if (lv && lv->full_type &&
              (lv->kind == TYPE_ARRAY || lv->kind == TYPE_POINTER))
              elem_type = lv->full_type->base;
          if (!elem_type) {
              GlobalVar *gv = codegen_find_global(cg, arr_base->value);
              if (gv && gv->full_type &&
                  (gv->kind == TYPE_ARRAY || gv->kind == TYPE_POINTER))
                  elem_type = gv->full_type->base;
          }
      }
      if (elem_type &&
          (elem_type->kind == TYPE_STRUCT || elem_type->kind == TYPE_UNION)) {
          StructField *f = find_struct_field(elem_type, node->value);
          if (f) {
              if (f->kind == TYPE_ARRAY && f->full_type && f->full_type->base) {
                  t = TYPE_POINTER;
                  node->full_type = type_pointer(f->full_type->base);
              } else if (type_is_fp(f->kind)) {
                  t = f->kind;
              } else {
                  t = (f->kind == TYPE_POINTER) ? TYPE_POINTER
                      : promote_kind(f->kind);
                  if (f->kind == TYPE_POINTER) node->full_type = f->full_type;
              }
          }
      }
  }
  ```
- Walks the subscript expression to its array variable, resolves the element type, finds the field, and returns its correct type (FP-aware).

### Step 7: Code Generator — Bug 3b: Add AST_ARRAY_INDEX base to sizeof_type_of_expr()

- Located `sizeof_type_of_expr()` AST_MEMBER_ACCESS case (~line 1854).
- Applied same AST_ARRAY_INDEX base handler block (without `node->full_type =` mutations since sizeof_type_of_expr does not modify the AST).
- Also added FP-aware `else if (type_is_fp(f->kind))` branches to the two existing base cases in that function for consistency.

### Step 8: Version

- Bumped `VERSION_STAGE` in `src/version.c` to `"01170000"`.

### Step 9: Tests

- Added 6 new valid test files in `test/valid/structs/`:
  - `test_struct_double_field_arith__4.c` — local struct with double field arithmetic (exit 4).
  - `test_struct_ptr_double_field_arith__4.c` — pointer-to-struct with double field via arrow (exit 4).
  - `test_struct_array_double_field__10.c` — global struct array with double field via subscript-then-dot (exit 10).
  - `test_struct_array_double_update__42.c` — struct array double field updated via addition (exit 42).
  - `test_struct_float_field_arith__3.c` — local struct with float field arithmetic (exit 3).
  - `test_struct_double_field_sub__42.c` — global struct array double field subtraction (exit 42).

### Step 10: Self-hosting

- Executed `./build.sh --mode=self-host` (C0→C1→C2 cycle).
- C0 (GCC-built): all 1863 tests pass.
- C1 (C0-bootstrapped): all 1863 tests pass.
- C2 (C1-bootstrapped): all 1863 tests pass.
- No source changes needed during bootstrap; compiler's own source uses local double variables for arithmetic, not struct FP fields.
- Updated `docs/self-compilation-report.md` with stage-117 results.

## Final Results

All 1863 tests pass at C0, C1, and C2 (1181 valid, 258 invalid, 88 integration, 50 print-AST, 100 print-tokens, 21 print-asm; 165 unit). No regressions. Self-hosting cycle completes successfully with no source modifications required.

## Session Flow

1. Read stage spec to understand three bugs affecting FP struct field reads and type inference.
2. Reviewed `codegen_expression()` AST_MEMBER_ACCESS and AST_ARROW_ACCESS rvalue blocks to identify the incorrect load instructions.
3. Reviewed `expr_result_type()` to understand the TYPE_INT fallback for FP struct fields and the missing AST_ARRAY_INDEX handler.
4. Applied Bug 1a fix to AST_MEMBER_ACCESS rvalue block: added FP early-return before `int sz = ...` calculation.
5. Applied Bug 1b fix to AST_ARROW_ACCESS rvalue block: identical FP early-return.
6. Applied Bug 2a, 2b, 2c fixes to `expr_result_type()`: added FP-aware branches to VAR_REF, DEREF, and ARROW_ACCESS cases.
7. Applied Bug 3a fix to `expr_result_type()` AST_MEMBER_ACCESS: added AST_ARRAY_INDEX base handler that walks to the array variable and resolves the field type.
8. Applied Bug 3b fix to `sizeof_type_of_expr()`: added same AST_ARRAY_INDEX handler plus FP-aware branches to existing cases.
9. Bumped version in `src/version.c` to `"01170000"`.
10. Built compiler and ran `test/valid/run_tests.sh` to verify existing tests still pass.
11. Created 6 new valid test files covering all FP struct field patterns: local struct dot access, pointer arrow access, global struct array subscript-then-dot access, field updates, float type, and subtraction.
12. Ran full test suite `./test/run_all_tests.sh` — all 1863 tests pass.
13. Executed self-hosting cycle `./build.sh --mode=self-host` — C0→C1→C2 all pass with no compiler modifications.
14. Updated `docs/self-compilation-report.md` with stage-117 checkpoint results.

## Notes

Writing to FP struct fields already worked correctly; the assignment path in `codegen_expression()` had existing `if (type_is_fp(f->kind))` guards that emitted `movss [rbx], xmm0` / `movsd [rbx], xmm0`. Only the read (rvalue) path was broken. Bug 2 is the key to why binary operators like `a.x - b.x` were falling through to the integer arithmetic path: `promote_kind(TYPE_DOUBLE)` returns TYPE_INT, causing the binary op dispatch to skip the FP code path entirely. Bug 3 is specific to subscript-then-member patterns where the base is an AST_ARRAY_INDEX node; neither existing VAR_REF nor DEREF condition matched, so the type stayed at the default TYPE_INT. All three bugs had to be fixed together for the benchmark pattern (global struct array with field arithmetic) to produce the correct exit code.
