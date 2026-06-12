# Milestone Summary

## Stage 103: Block-Scope Static Scalar Constant-Expression Initializers - Complete

stage-103 extends block-scope static scalar initializers to accept the full set of compile-time constant expressions, matching the expressiveness already supported for `case` labels, enum values, and file-scope globals.

- Tokenizer: No changes.
- Grammar/Parser: No changes.
- AST/Semantics: No changes.
- Codegen: Added `eval_const_init` static helper in `src/codegen.c` before `codegen_statement` to recursively evaluate parsed AST nodes as compile-time integer constants. Replaces the 3-case ad-hoc check with a single call. Handles: `AST_INT_LITERAL` (with base-0 hex support), `AST_CHAR_LITERAL`, `AST_SIZEOF_TYPE`, unary operators (`-`, `~`, `!`, `+`), and binary operators (`+`, `-`, `*`, `/`, `%` with div-by-zero guard, `<<`, `>>`, `&`, `^`, `|`). Two bugs fixed: hex literals needed base-0 conversion, and scalar `sizeof` needed `type_kind_bytes` fallback when `full_type` is NULL. Version bump: `VERSION_STAGE` to `01030000` in `src/version.c`.
- Tests: 7 new valid tests, 2 new invalid tests added. Full suite 1569/1569 pass (894 valid, 253 invalid, 86 integration, 50 print-AST, 100 print-tokens, 21 print-asm; 165 unit).
- Docs: Updated README.md ("Through stage 103"), refreshed test totals, extended static-variable bullet for constant-expression initializers.
- Notes: Self-host C0→C1→C2 all pass 1569 tests. No compiler source changes needed.
