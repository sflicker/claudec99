# Milestone Summary

## Stage 20-02: Comma-Separated Init-Declarator Lists - Complete

Stage 20-02 ships support for comma-separated init-declarator lists, allowing multiple declarators in a single declaration statement (e.g., `int a, b;`, `int a=3, b=4;`, `int *p, q;`).

- **Tokenizer**: No changes required.
- **Grammar/Parser**: Extended `parse_statement()` declaration branch to loop on `TOKEN_COMMA` after parsing the first init-declarator. Each declarator shares the base type but may have independent pointer depth and initializers. Multiple declarators are wrapped in `AST_DECL_LIST`; single declarators return `AST_DECLARATION` as before.
- **AST/Semantics**: Added `AST_DECL_LIST` node type to represent a list of declarations. Type checking and initialization semantics remain unchanged.
- **Codegen**: Added `AST_DECL_LIST` handler in `codegen_statement()` that iterates children and code-generates each. `compute_decl_bytes` already handles recursive traversal.
- **Tests**: 5 new tests (all valid); full suite **614 passed** (381 valid, 102 invalid, 24 print-AST, 88 print-tokens, 19 print-asm). No regressions.
- **Docs**: Updated `docs/grammar.md` to replace single-declarator rule with init-declarator-list rule; removed "Only one declarator per declaration" restriction.
- **Notes**: Array declarators in multi-declarator lists are rejected with a clear error message (out of scope for this stage). Spec file title matches stage-20-02.
