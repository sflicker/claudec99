# Stage 109 — Milestone Summary

**Stage:** Stage 109 — float and double types, literals, and stack variables
**Date:** 2026-06-13
**Status:** Complete

## What Was Implemented

Stage 109 adds `float` and `double` as first-class scalar types across the full compiler pipeline.

### Tokenizer
- New keyword tokens: `TOKEN_FLOAT`, `TOKEN_DOUBLE`
- New literal tokens: `TOKEN_FLOAT_LITERAL`, `TOKEN_DOUBLE_LITERAL`
- Lexer scans FP literals in all standard decimal forms: digits with optional `.fraction`, optional `e`/`E` exponent, optional `f`/`F` suffix; also leading-dot form (`.5f`)

### Type System
- `TYPE_FLOAT` (size=4, align=4) and `TYPE_DOUBLE` (size=8, align=8) added to `TypeKind`
- Static singletons `type_float_singleton` / `type_double_singleton`
- `type_is_integer()` returns 0 for both; `type_kind_name()` returns "float" / "double"

### AST
- `AST_FLOAT_LITERAL` node: single node type for both float and double; `decl_type` field distinguishes them

### Parser
- `parse_type_specifier()` handles `TOKEN_FLOAT` / `TOKEN_DOUBLE`
- `parse_primary()` creates `AST_FLOAT_LITERAL` from `TOKEN_FLOAT_LITERAL` / `TOKEN_DOUBLE_LITERAL`
- All type-start lookahead sets updated (compound literal, sizeof, cast, for-init, statement dispatch, file-scope dispatch, const-expr sizeof)
- File-scope FP initializers accepted via `parse_assignment_expression()` + `AST_FLOAT_LITERAL` validation

### Code Generation
- `FpLiteral` typedef (raw_text, label, is_double) with deduplication by raw text
- Load/store helpers: `emit_load_fp_local/global()`, `emit_store_fp_local/global()`
- Implicit widening: `emit_fp_widen_if_needed()` emits `cvtss2sd xmm0, xmm0` when float→double
- `AST_FLOAT_LITERAL`: interns literal into pool, emits `movss`/`movsd xmm0, [rel Lfc<N>]`
- `AST_VAR_REF`: FP path uses `movss`/`movsd` load for local/global/static
- `AST_DECLARATION`: FP local allocated at proper size/alignment, initialized from xmm0
- `AST_ASSIGNMENT`: FP path for local, global, member-dot, member-arrow (with push/pop of address)
- Global FP: `codegen_add_global()` stores raw text in init_label; `codegen_emit_data()` emits `g: dd/dq <value>` stripping f/F suffix; `bss_res_directive()` returns `resd`/`resq`
- `codegen_emit_fp_literals()`: emits `section .rodata` with `Lfc<N>: dd/dq <value>` entries; called from `codegen_translation_unit()`

## Tests

Six new tests added to `test/valid/`:

| Test | What it exercises |
|------|-------------------|
| `test_float_declare__0.c` | `float x = 1.5f;` — float local declaration and initialization |
| `test_double_declare__0.c` | `double x = 3.14;` — double local declaration and initialization |
| `test_float_copy__0.c` | `float y = x;` — float-to-float local copy |
| `test_double_widen__0.c` | `double d = f;` — implicit float→double widening via cvtss2sd |
| `test_float_struct_member__0.c` | Struct with two float members, assigned via movss through member address |
| `test_double_global__0.c` | `double g = 2.718;` — initialized double global; loaded into local |

**All 1627 tests pass** (942 valid, 255 invalid, 86 integration, 50 print-AST, 100 print-tokens, 21 print-asm; 165 unit).

## Self-Host Result

C0 → C1 → C2 bootstrap completed cleanly. The compiler's own source uses no `float` or `double`, so all new code paths are inert during bootstrap.

| Step | Version | Tests |
|------|---------|-------|
| C0 (GCC-built) | `00.02.01090000.00867` | 1627/1627 |
| C1 (C0-built) | `00.02.01090000.00868` | 1627/1627 |
| C2 (C1-built) | `00.02.01090000.00869` | 1627/1627 |

## Files Changed

`include/token.h`, `include/type.h`, `include/ast.h`, `include/codegen.h`, `src/type.c`, `src/lexer.c`, `src/parser.c`, `src/codegen.c`, `src/ast_pretty_printer.c`, `src/version.c`, `docs/self-compilation-report.md`, `docs/grammar.md`, `docs/outlines/checklist.md`, `README.md`
