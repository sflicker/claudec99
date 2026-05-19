# Milestone Summary

## Stage 55-04: Unary Preprocessor Operators - Complete

stage-55-04 ships unary `!`, `-`, and `+` operator support in `#if` and `#elif` preprocessor condition expressions.

- Preprocessor: Added two helpers to `src/preprocessor.c` — `eval_cond_primary()` (parses `defined(...)`, integer literals, and object-like macro identifiers) and `eval_cond_expr()` (collects leading unary operators and applies them innermost-first). Replaced the duplicated ~60-line inline condition parsing blocks in both the `#if` and `#elif` handlers with calls to these helpers, eliminating duplication while adding the new capability. Chained unary operators are supported (e.g. `!-1`).
- Parser/Semantics: No changes to the main language grammar.
- Tests: 11 new valid tests covering all operator/operand combinations including chained `!-1`. Full suite **966/966 pass** (588 valid, 192 invalid, 27 integration, 39 print-AST, 99 print-tokens, 21 print-asm).
- Docs: Grammar `<if-condition>` rule expanded to `<pp-unary-expression>` → `<pp-primary>`. README updated.
- Notes: Spec tests 4 & 5 had incorrect "expect status code 1" comments — correct exit code is 42. Tests 6–11 had stray `do #endif` (copy-paste artifact). Both issues resolved in the implemented tests.
