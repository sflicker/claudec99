# Milestone Summary

## Stage 114: Fix Issues with Legacy Valid Tests - Complete

Stage 114 fixes 24 failing tests from a legacy project imported into test/valid/legacy_project/, addressing 4 test file issues and 7 compiler bugs, then migrates all 219 legacy tests to appropriate category subfolders.

- **Tokenizer**: No changes.
- **Grammar/Parser**: Added `AST_STRING_LITERAL` to allowed subscript base expressions, enabling `"abc"[2]` to compile.
- **AST/Semantics**: No changes.
- **Codegen**: 7 fixes — (1) FP array subscript WRITE: emit `movss`/`movsd` instead of `mov` for float/double elements; (2) FP array subscript READ: emit `movss`/`movsd` instead of truncating double bits via `movsxd`; (3) `expr_result_type` for array subscripts: return `element->kind` directly for FP types instead of promoting to `TYPE_LONG`; (4) nested brace local array init: add `emit_local_array_init()` for recursive `AST_INITIALIZER_LIST` handling in multidimensional arrays; (5) nested brace global array init: add `emit_global_array_elements()` for sub-brace grouping; (6) mixed FP/int ternary: pre-compute both branch types, find common FP type, call `emit_fp_widen_if_needed()` after each branch; (7) string literal subscript codegen: add `AST_STRING_LITERAL` case in `emit_array_index_addr()`.
- **Tests**: 4 test files fixed (3 renamed for exit code overflow, 1 modified for invalid array initializer); 191 unique legacy tests migrated to 13 category subfolders (arithmetic, arrays, bitwise, casting, comparisons, control_flow, declarations, expressions, floating_point, functions, misc, pointers, strings); 28 legacy tests already present in target directories (duplicates merged); test/valid/legacy_project/ deleted. All 1841 tests pass (1161 valid, 256 invalid, 88 integration, 50 print-AST, 100 print-tokens, 21 print-asm; 165 unit).
- **Docs**: README.md updated with test totals and stage 114 capability entry.
- **Notes**: Self-host C0→C1→C2 all pass 1841/1841 with no compiler source changes needed during bootstrap. Legacy test suite fully integrated into permanent test structure.
