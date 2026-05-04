# Milestone Summary

## Stage 18: Conditional Operator - Complete

stage-18 ships the conditional operator (ternary `?:`), enabling selection between two expressions based on a condition.

- **Tokenizer**: Added `TOKEN_QUESTION` for the `?` token; both added to `token.h` and included in `compiler.c`'s `token_type_name()`.
- **Grammar/Parser**: Inserted `<conditional_expression>` production between `<assignment_expression>` and `<logical_or_expression>` with right-associative recursion on the false branch. `parse_conditional()` called from `parse_expression()` after `parse_logical_or()`.
- **AST/Semantics**: Added `AST_CONDITIONAL_EXPR` node type. Result type computed from true/false branch evaluation; null-pointer-constant detection via `is_null_pointer_constant()` on AST node. Condition comparison uses 64-bit (`cmpq`/`jne`) for `TYPE_LONG` and `TYPE_POINTER`, 32-bit (`cmpl`/`jne`) for integer types.
- **Codegen**: Condition evaluated; unique labels (`.L<cond>_true`, `.L<cond>_false`, `.L<cond>_end`) control branch selection. Only the selected branch is evaluated; comparison logic inlined (matching `&&`/`||` pattern).
- **Tests**: 10 new valid tests, 4 new invalid tests. Suite counts: 367 valid, 101 invalid, 24 print-AST, 88 print-tokens, 19 print-asm. Full suite 597/597 pass.
- **Docs**: Grammar updated with `<conditional_expression>` production; README updated with stage 18 reference, feature description, and test counts.
- **Notes**: Spec had two typos — `?:` test case used `:` instead of `?` (treated as `x ? (1/y) : 20`); grammar rule used `::` instead of `::=`.
