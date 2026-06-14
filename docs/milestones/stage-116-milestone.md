# Milestone Summary

## Stage 116 — Global Struct Array BSS Fix and Char[N] String-Literal Initialization - Complete

stage-116 ships two codegen bug fixes addressing silent data corruption when global (file-scope) struct/union arrays are used with or without string initialization in embedded char[N] fields.

- **Codegen — Bug 1 (BSS sizing)**: `codegen_emit_bss()` and `codegen_emit_local_statics()` single-dimension else-branch changed from `resd N` (element-count directive for `int`) to `resb full_type->size` (byte-count directive for total array size). Fixes struct/union array BSS allocation that was undersizing by emitting element count instead of full byte size.
- **Codegen — Bug 2 (String initialization)**: Added `emit_string_as_bytes()` helper to emit string literals as inline bytes in three sites: `codegen_emit_data()` global array loop detects `char[N]` element type and emits bytes inline instead of pointer; `emit_global_array_elements()` recursive helper added char[N]-from-string branch; `emit_global_struct()` field handler added char[N]-from-string branch for struct fields.
- **Tests**: 7 new valid tests added (4 BSS-fix tests for global/static struct arrays, 2 char[2D] tests for 2-D char array string initialization, 1 struct-char-array field test). Expected file updated (`test_stage_22_01_global_bss_layout.expected`). Full suite: 1857 tests pass (1175 valid, 258 invalid, 88 integration, 50 print-AST, 100 print-tokens, 21 print-asm, 165 unit).
- **Version**: Bumped to `"01160000"` in `src/version.c`.
- **Self-host**: C0→C1→C2 passes cleanly; all 1857 tests pass at each stage with no source changes needed during bootstrap.
- **Docs**: Self-compilation report updated with stage-116 result.
- **Notes**: Single-dimension array BSS emission for scalar types (`int arr[N]`, `char arr[N]`) now emits `resb total` instead of `resd N` (semantically identical for all types). All global struct array patterns (uninitialized, initialized with struct literals, embedded char[N] fields) now work correctly.
