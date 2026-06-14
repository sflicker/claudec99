# Milestone Summary

## Stage 125: FP Global Init and Variadic Float Promotion - Complete

Stage 125 fixes two floating-point correctness bugs and closes stale checklist items.

### FP Globals from Integer Initializers

- **Parser** (`src/parser.c`): Extended the `TYPE_FLOAT`/`TYPE_DOUBLE` global initializer branch to accept `AST_INT_LITERAL` in addition to `AST_FLOAT_LITERAL`. Negated integer literals (e.g., `double x = -7;`) are folded: the parser collapses `AST_UNARY_OP("-", AST_INT_LITERAL("7"))` to `AST_INT_LITERAL("-7")` by converting via `strtol`/`snprintf`/`lexer_store_bytes`.
- **Codegen** (`src/codegen.c`): In `codegen_add_global`, the `AST_INT_LITERAL` case now checks if the global kind is `TYPE_FLOAT` or `TYPE_DOUBLE`. If so, the integer is converted to a decimal string via `snprintf` with `%.17g` (double) or `%.9g` (float), and `.0` is appended if no decimal point or exponent is present. The result is stored via `init_label` (the same path as float/double literals), so NASM interprets `dq 5.0` as IEEE 754 double encoding, not raw integer bits.
- **Tests**: 4 new valid tests (`test_double_global_from_int__0.c`, `test_float_global_from_int__0.c`, `test_double_global_from_zero__0.c`, `test_double_global_negative_from_int__0.c`).

### Variadic Float→Double Promotion

- **Codegen** (`src/codegen.c`): The `involves_special` call-emission path was extended to promote `float` arguments to `double` in variadic function calls per C99 §6.5.2.2p7. Detection uses the existing `compute_call_layout` mechanism: variadic float extras are assigned `s->nbytes = 8` (not 4), signaling promotion. Both Phase 1 (stack-overflow) and Phase 2 (XMM-register) paths now emit `cvtss2sd xmm0, xmm0` for `float` variadic args before the store. The check is `callee && callee->is_variadic && actual_types[i] == TYPE_FLOAT`.
- **Tests**: 3 new valid tests (`test_variadic_float_printf__0.c` with `.expected`, `test_variadic_float_direct__0.c` with `.expected`, `test_variadic_int_unaffected__5.c`).

### Checklist Cleanup

- **Docs** (`docs/outlines/checklist.md`): Two items already implemented in Stage 110 are now marked complete:
  - "Unary + on floating-point" → `[x] Unary + on floating-point (Stage 110)`
  - "Mixed integer/floating-point arithmetic" → `[x] Mixed integer/floating-point arithmetic (usual arithmetic conversions) (Stage 110)`

### Self-Host Bootstrap

C0 → C1 → C2 bootstrap cycle verified with no source changes needed. All 1919 tests pass at each stage.

## Final Results

All 1919 tests pass (7 new tests added):
- 1233 valid tests (was 1226; +7)
- 262 invalid tests (unchanged)
- 88 integration tests (unchanged)
- 50 print-AST tests (unchanged)
- 100 print-tokens tests (unchanged)
- 21 print-asm tests (unchanged)
- 165 unit tests (unchanged)

Self-host verification: C0 → C1 → C2 all pass 1919/1919 tests with no compiler source changes.

## Notes

The integer-to-FP conversion fix is essential for correctness: without it, `double x = 5;` at file scope emitted `dq 5` (integer bits), not the IEEE 754 encoding. The parser folding of negated integer literals avoids creating intermediate `AST_UNARY_OP` nodes for simple cases. The variadic float promotion fix enforces C99 §6.5.2.2p7, which requires `float` arguments in variadic calls to be promoted to `double`. Detection via `nbytes == 8` is clean because `compute_call_layout` already assigned the correct byte count for all arg classes.
