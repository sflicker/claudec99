# Stage 126–130 Kickoff — Tentative Definitions, Callee-Saved Registers, FP Expressions, Block-Scope Functions, and Variadic Struct Types

## Overview

Five stages address missing C99 language features and ABI correctness bugs:

1. **Tentative definitions** (C99 §6.9.2) — `int x; int x = 5;` currently errors with "duplicate global"
2. **Callee-saved registers r12–r15** — SysV AMD64 ABI requires four additional registers to be preserved
3. **FP constant expressions at file scope** — `double TWO_PI = 3.14159 * 2.0;` currently errors
4. **Block-scope function declarations and extern incomplete arrays** — `{ void foo(void); }` and `extern int arr[];` currently error
5. **`va_arg` for struct/union by value** — `va_arg(ap, struct T)` currently errors with "aggregate types not supported"

These stages are largely independent; block-scope function declarations depend on the parser infrastructure, and Stage 130 depends on call-layout logic developed in earlier stages.

## Spec Summary

### Stage 126 — Tentative Definitions (C99 §6.9.2)

**Problem:** C99 allows multiple tentative definitions of the same global (initializer-less declarations), and a single definition (with initializer) to finalize the name. The compiler currently rejects any re-declaration of a global name.

**Fix:** 
- Parser: Add `is_defined` field to `GlobalObjSig` to track which declarations have initializers.
- `parser_register_global()` gains `has_init` parameter; re-registration is allowed if the existing entry has no initializer.
- Codegen: `codegen_add_global()` gains tentative fast-path: if `GlobalVar` already exists and new declaration has no children (no initializer), return silently (duplicate tentative) or clear `is_extern` (for `extern T x; T x;` upgrade). If new has initializer, update existing entry in-place.

**Tests:** `test_tentative_def_then_init__0.c`, `test_tentative_def_init_then_tentative__0.c`, `test_tentative_def_struct__0.c`, `test_tentative_two_external__0.c`, `test_tentative_two_static__0.c`, `test_tentative_two_static_fnptr__0.c`.

### Stage 127 — Callee-Saved Registers r12–r15

**Problem:** SysV AMD64 ABI mandates that registers rbx, r12–r15 be callee-saved. Only rbx is currently preserved.

**Fix:**
- Stack-frame formula changes from `+ 8` to `+ 40` (5 callee-saved slots × 8 bytes).
- `cg->stack_offset` initialized to 40 instead of 8.
- Prologue: add `mov [rbp - 16..40], r12..r15` after rbx save.
- Epilogue: restore all four registers in all paths.
- All `.expected` files for print_asm tests regenerated (offsets shift by 32 bytes).

**Tests:** `test_callee_saved_r12_r15__0.c`.

### Stage 128 — FP Constant Expressions at File Scope

**Problem:** Only single float/double literals accepted as file-scope initializers. `double TWO_PI = 3.14159 * 2.0;` errors.

**Fix:**
- Parser: Add recursive-descent FP constant evaluator (`eval_fp_primary`, `eval_fp_unary`, `eval_fp_mult`, `eval_fp_const_expr`) in `src/parser.c`.
- Float/double global initializer path calls `eval_fp_const_expr` instead of `parse_assignment_expression`.

**Tests:** `test_double_global_arith__0.c`, `test_float_global_arith__0.c`, `test_double_global_neg_lit__0.c`, `test_double_global_expr_parens__0.c`.

### Stage 129 — Block-Scope Function Declarations and Extern Incomplete Arrays

**Feature 1 — Block-scope function declarations:** `void foo(void);` inside `{}` previously errored. Parser detects a function-type declarator inside a block, skips the parameter list with a depth counter, emits `AST_TYPEDEF_DECL` as a no-op, and registers the function with param_count `-1` (unknown arity). Call-site arity check skips when `sig->param_count == -1`.

**Feature 2 — Extern incomplete arrays:** `extern int arr[];` previously errored. Parser allows `has_size=1, length=0` for `SC_EXTERN` declarations. Codegen update path sets `existing_gv->full_type = decl->full_type` so `emit_global_array_elements` uses the completing definition's length.

**Tests:** `test_block_scope_fn_decl__0.c`, `test_block_scope_fn_decl_forward__0.c`, `test_extern_incomplete_array__0.c`.

### Stage 130 — `va_arg` for Struct/Union by Value

**Problem:** `va_arg(ap, struct T)` errors with "aggregate types are not supported". Requires implementing SysV AMD64 register classification for struct arguments.

**Three-case implementation in `codegen_add_global`'s `AST_BUILTIN_VA_ARG` handler:**
- **MEMORY class** (size > 16 bytes): read from `overflow_arg_area`, copy to scratch slot with `rep movsb`, advance overflow pointer by `align8(size)`.
- **Register class 1 eightbyte** (size ≤ 8): check `gp_offset < 48`; load from `reg_save_area + gp_offset` or `overflow_arg_area`; copy 8 bytes to scratch.
- **Register class 2 eightbytes** (size 9–16): check `gp_offset ≤ 32`; load two consecutive 8-byte slots; advance appropriately.

**Variadic struct argument fix:** `compute_call_layout` was setting `is_struct = p && ...` (always 0 for variadic extras). Fixed by: (1) extend `expr_result_type` to return `TYPE_STRUCT`/`TYPE_UNION` for struct vars; (2) add `CodeGen *cg` parameter to `compute_call_layout` and look up struct's `full_type` from var tables; (3) forward-declare helper functions; (4) update `involves_special` check to detect struct args.

**Tests:** `test_va_arg_struct_memory__0.c`, `test_va_arg_struct_reg1__0.c`, `test_va_arg_struct_reg2__0.c`; moved from invalid: `test_va_arg_struct_by_value__0.c`.

---

## Required Changes

### Parser (`src/parser.c`)

- **Stage 126:** Add `int is_defined` to `GlobalObjSig`; extend `parser_register_global()` with `has_init` parameter.
- **Stage 128:** Add FP constant expression evaluator functions and call them for float/double global initializers.
- **Stage 129:** Detect and skip block-scope function declarations; allow `extern` incomplete arrays.

### AST (`include/ast.h`)

- **Stage 126:** Add `is_defined` field to `GlobalObjSig`.

### Codegen (`src/codegen.c`)

- **Stage 126:** Tentative fast-path in `codegen_add_global()` for duplicate/upgrade cases.
- **Stage 127:** Update stack-frame computation and prologue/epilogue for r12–r15.
- **Stage 130:** Extend `compute_struct_ret_bytes` for `AST_BUILTIN_VA_ARG` struct nodes; replace TYPE_STRUCT error case in va_arg handler; implement three-case struct va_arg; extend `emit_struct_addr` for va_arg case; fix `expr_result_type` to preserve struct/union kinds; add `cg` parameter to `compute_call_layout` and struct full-type lookup; update involves_special check.

### Version (`src/version.c`)

Bump VERSION_STAGE from `"01250000"` to `"01300000"`.

---

## Test Plan

**Stage 126:** 6 valid tests covering tentative redeclaration, initialization ordering, and static/extern variations.

**Stage 127:** 1 valid test verifying r12–r15 callee-save. Regenerate all 21 print_asm `.expected` files.

**Stage 128:** 4 valid tests covering double/float arithmetic, negation, and parentheses.

**Stage 129:** 3 valid tests covering block-scope function forward declaration, and extern incomplete array completion.

**Stage 130:** 4 valid tests covering struct memory class, 1-eightbyte register class, 2-eightbyte register class, and a moved-from-invalid test for struct-by-value va_arg.

---

## Implementation Order

1. Stage 126: Parser `is_defined` field, `parser_register_global()` signature, codegen tentative fast-path, tests.
2. Stage 127: Stack-frame formula, prologue/epilogue, test, regenerate `.expected` files.
3. Stage 128: FP constant expression evaluator, parser integration, tests.
4. Stage 129: Block-scope function detection, extern incomplete array, tests.
5. Stage 130: `compute_struct_ret_bytes`, va_arg struct implementation, `expr_result_type` fixes, `compute_call_layout` enhancements, `emit_struct_addr` case, tests, move invalid test to valid.
6. Version bump to `01300000`.
7. Full test suite: `test/valid/run_tests.sh`, `test/invalid/run_tests.sh`, `test/print_asm/run_tests.sh`.
8. Self-host: C0 → C1 → C2 cycle.
9. Commit with Co-Authored-By trailer.

---

## Notes & Decisions

- **Tentative definitions:** The parser controls whether a declaration is tentative or defining via the `has_init` parameter. Codegen never needs to know the distinction; the parser enforces it.
- **Callee-saved registers:** ABI compliance requires saving/restoring all five callee-saved slots unconditionally. This affects all compiled functions.
- **FP expressions:** Constant evaluation at parse time simplifies codegen. No need to emit runtime code for file-scope initializers.
- **Block-scope functions:** Treated as no-ops (emit `AST_TYPEDEF_DECL`). Unknown arity (`-1`) allows flexible calling conventions.
- **Extern incomplete arrays:** Update the existing global's full_type at definition time so emitted array size matches the completing declaration.
- **Struct va_arg:** SysV AMD64 register classification (1–2 eightbytes, memory) is essential for correctness. Lookup of struct full_type in symbol tables at call-layout time ensures variadic struct args are classified correctly.
