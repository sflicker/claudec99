# Milestone Summary

## Stage 55-09: Bitwise and Shift Operators in Preprocessor - Complete

stage-55-09 ships support for bitwise (`~`, `&`, `^`, `|`) and shift (`<<`, `>>`) operators in preprocessor `#if`/`#elif` conditional expressions.

- **Preprocessor**: Extended `eval_cond_*` expression parser to handle shift operators (inserted between additive and relational) and bitwise operators (AND, XOR, OR with precedence between equality and logical-AND, respecting the standard `&` > `^` > `|` ordering). Unary complement `~` added to existing unary handler. Macro replacement integer parser extended to handle negative literals (e.g., `#define VALUE -1`).
- **Tests**: 10 new tests in `test/valid/` covering unary complement, bitwise AND/XOR/OR, left/right shifts, chained shifts, and mixed-precedence expressions. Full suite **1004 passed, 0 failed, 1004 total** (was 994).
- **Docs**: `docs/grammar.md` preprocessor expression rules updated to document shift operators and bitwise operators with correct precedence. README.md stage reference updated to 55-09 with revised preprocessor capabilities and test totals.
- **Notes**: Spec's `~VALUE` test required clarification: changed `#define VALUE 1` to `#define VALUE -1` so that `~(-1) = 0` (falsy) matches the expected exit code. Missing `#endif` in one test case added.

