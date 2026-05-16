# Milestone Summary

## Stage 51: Function-like Macros - Complete

stage-51 ships function-like `#define` macro support with parameter substitution, nested invocation, and argument-count validation.

- Preprocessor: Extended `MacroDef` struct with parameter arrays (param_count field: -1 for object-like, >=0 for function-like); added `collect_args()` to parse argument lists at depth-0 comma splits; added `substitute_args()` to replace parameter names in replacement text; added `expand_macros_text()` for recursive macro expansion in plain text; extended `#define` parsing to detect and parse zero-or-more parameter lists when `(` immediately follows the macro name.
- Semantics: Function-like macros require exact argument-count matching; arguments may contain arbitrary token sequences including nested parentheses; arguments are pre-expanded before substitution; nested macro invocations are recursively expanded.
- Tests: 6 new tests added (5 valid, 1 invalid). Valid: `ADD(20,22)` returns 42; `SQUARE(8)` returns 64; zero-arg `ANSWER()` returns 42; `ADD(1+2,3)` returns 6; nested `ADD(ADD(1,2),3)` returns 6. Invalid: argument-count mismatch rejected.
- Final results: 913 total tests pass (547 valid, 182 invalid, 25 integration, 39 print-AST, 99 print-tokens, 21 print-asm). No regressions.
- Notes: Spec typo corrected in `SQUARE` test: replacement is `((x)*(x))` not `((x)*(X))`.
