# Milestone Summary: Stages 126–130 Complete

Five stages completed in a single session and committed as f9bf3ee.

## Stage 126 — Tentative Definitions (C99 §6.9.2)

- **Parser** (`include/parser.h`, `src/parser.c`): Added `int is_defined` field to `GlobalObjSig`. `parser_register_global()` now takes `has_init` parameter; re-registration of the same name is allowed when the existing entry has no initializer.
- **Codegen** (`src/codegen.c`): `codegen_add_global()` gained tentative fast-path: if a `GlobalVar` already exists and the new declaration has no children (no initializer), it either silently returns (duplicate tentative) or clears `is_extern` (for `extern T x; T x;` upgrade). If the new declaration has an initializer, it updates the existing entry in-place (init fields, type, linkage, size).
- **Tests (6):** `test_tentative_def_then_init__0.c`, `test_tentative_def_init_then_tentative__0.c`, `test_tentative_def_struct__0.c`, `test_tentative_two_external__0.c`, `test_tentative_two_static__0.c`, `test_tentative_two_static_fnptr__0.c`; moved from invalid: `test_invalid_dup_def__duplicate_definition.c`.

## Stage 127 — Callee-Saved Registers r12–r15 (SysV AMD64 ABI)

- **Codegen** (`src/codegen.c`): Stack-size formula changed from `+ 8` to `+ 40` (5 callee-saved slots × 8 bytes). `cg->stack_offset` initialized to 40 instead of 8. Added `mov [rbp - 16..40], r12..r15` in prologue after rbx save; added 4-register restore in all epilogue paths.
- **Print-asm tests:** All 21 `.expected` files regenerated (local variable offsets shifted by 32 bytes).
- **Tests (1):** `test_callee_saved_r12_r15__0.c`.

## Stage 128 — FP Constant Expressions at File Scope

- **Parser** (`src/parser.c`): Added recursive-descent FP constant evaluator (`eval_fp_primary`, `eval_fp_unary`, `eval_fp_mult`, `eval_fp_const_expr`). The float/double global initializer path now calls `eval_fp_const_expr` instead of `parse_assignment_expression`.
- **Tests (4):** `test_double_global_arith__0.c`, `test_float_global_arith__0.c`, `test_double_global_neg_lit__0.c`, `test_double_global_expr_parens__0.c`.

## Stage 129 — Block-Scope Function Declarations and Extern Incomplete Arrays

**Feature 1 — Block-scope function declarations:**
- **Parser** (`src/parser.c`): Detects a function-type declarator inside a block, skips the parameter list with a depth counter, emits `AST_TYPEDEF_DECL` as a no-op, and registers the function with param_count `-1` (unknown arity). Call-site arity check skips when `sig->param_count == -1`.
- **Tests (2):** `test_block_scope_fn_decl__0.c`, `test_block_scope_fn_decl_forward__0.c`.

**Feature 2 — Extern incomplete arrays:**
- **Parser** (`src/parser.c`): Allows `has_size=1, length=0` for `SC_EXTERN` declarations. 
- **Codegen** (`src/codegen.c`): Update path sets `existing_gv->full_type = decl->full_type` so `emit_global_array_elements` uses the completing definition's length.
- **Tests (1):** `test_extern_incomplete_array__0.c`.

## Stage 130 — `va_arg` for Struct/Union by Value (SysV AMD64)

- **Codegen struct va_arg handler** (`src/codegen.c`): Implemented three-case SysV AMD64 ABI classification:
  - **MEMORY class** (size > 16 bytes): read from `overflow_arg_area`, copy to scratch slot with `rep movsb`, advance overflow pointer.
  - **Register class 1 eightbyte** (size ≤ 8): check `gp_offset < 48`; load from `reg_save_area + gp_offset` or overflow; copy to scratch.
  - **Register class 2 eightbytes** (size 9–16): check `gp_offset ≤ 32`; load two consecutive 8-byte slots; advance appropriately.

- **`compute_struct_ret_bytes` extension** (`src/codegen.c`): Pre-allocates scratch slots for `AST_BUILTIN_VA_ARG` struct nodes.

- **`emit_struct_addr` extension** (`src/codegen.c`): Added `AST_BUILTIN_VA_ARG` case so `struct S s = va_arg(ap, S)` works.

- **Variadic struct argument fix** (`src/codegen.c`): 
  - `compute_call_layout` was setting `is_struct = p && ...` (always 0 for variadic extras). 
  - Fixed by: (1) `expr_result_type` now returns `TYPE_STRUCT`/`TYPE_UNION` for struct local/global vars; (2) `compute_call_layout` gained `CodeGen *cg` parameter and looks up struct's `full_type` from var tables when `is_struct` detected via `actual_kind`; (3) forward declarations added for `codegen_find_var`/`codegen_find_global`; (4) `involves_special` check updated to detect struct args.

- **Tests (4):** `test_va_arg_struct_memory__0.c`, `test_va_arg_struct_reg1__0.c`, `test_va_arg_struct_reg2__0.c`; moved from invalid: `test_va_arg_struct_by_value__0.c`.

---

## Final Test Results

**All 1531 tests pass (0 failures):**
- 1251 valid (was 1246; +5 new, +1 moved from invalid)
- 259 invalid (was 260; -1 moved to valid)
- 21 print_asm (all `.expected` files regenerated for Stage 127)

**Version bumped to `01300000`.**

---

## Notes

The five stages form a natural progression through C99 compliance and ABI correctness:

- **Stage 126** fixes a parser-level ambiguity in the C standard (tentative vs. definite).
- **Stage 127** corrects ABI compliance for all compiled functions (callee-saved registers).
- **Stage 128** extends the parser's expression evaluator to handle constant FP arithmetic.
- **Stage 129** lifts two syntactic restrictions (block-scope functions, extern arrays) by recognizing new patterns.
- **Stage 130** implements the most complex code-generation task: SysV AMD64 struct classification and proper register/overflow handling for variadic struct arguments. This required integrating struct type information into the call-layout mechanism, which had previously treated all variadic extras as scalars.

All stages required no language-grammar changes, no new AST node types, and no lexer modifications. The improvements were surgical: parser validation rules, codegen data structures, and code-generation sequences.
