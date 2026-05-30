# Milestone Summary

## Stage 76: For-Loop Initializer Declarations - Complete

stage-76 ships support for C99 declarations in the `for` loop initializer clause.
- Grammar/Parser: Extended `<for_statement>` to use `<for_init>` rule; `parse_for_statement` now opens a scope before parsing init, detects type-specifier tokens, and dispatches to `parse_statement` for declarations or `parse_expression_statement` for expressions.
- AST/Semantics: For-loop scope created and destroyed around the entire loop; declared variables are in scope for condition, post-expression, and body.
- Codegen: Saves/restores `scope_start` and `local_count` to manage variable offsets across the loop boundary.
- Tests: 9 new valid tests (basic sum, body visibility, post visibility, multi-declarator, pointer, continue, shadowing), 2 new invalid tests (scope leak, void type), 1 new print-AST test. All 1211 tests pass.
- Docs: Updated README.md with stage 76 milestone and test count; updated grammar.md header.
- Notes: Spec contained several test cases with typos (missing semicolons, off-by-one indexing, garbled const-pointer test); all corrected during implementation.
