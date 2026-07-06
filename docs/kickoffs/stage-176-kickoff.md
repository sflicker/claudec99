# Stage-176 Kickoff

## Spec Summary

Stage 176 is a bugfix for a parser defect that rejects bare `;` (null statements) in the bodies of `while`, `for`, and other statement contexts. C99 §6.8.3 specifies that `expression_statement ::= [expression] ";"` — the expression is optional, allowing a lone semicolon to serve as a no-op statement. Example programs like `int main() { while(0); return 0; }` and `for(;;);` were rejected with `error: expected expression, got ';'` because the parser's `parse_statement()` function had no case for a bare semicolon and fell through to the final "expression_stmt" fallback, which unconditionally called `parse_expression()`. Since `parse_expression()` has no valid production starting with `;`, the error resulted.

## What changes from stage 175

No tokenizer, lexer, or codegen changes are needed. Parser only.

**Parser fix:**
- Add a check immediately before the expression-statement fallback in `parse_statement()` in `src/parser.c` to detect a bare `;` token.
- When found, consume the semicolon and return an empty `AST_BLOCK` node (the same convention already used elsewhere for GCC `__asm__` statements).
- No new AST node types or grammar productions required — just make the existing expression in `expression_statement` optional per the C99 spec.

**Grammar update:**
- `docs/grammar.md` line for `<expression_statement>` changes from `<expression> ";"` to `[ <expression> ] ";"` to reflect that the expression is now optional.

## Root Cause Analysis

The parser's `parse_statement()` function had a series of branches for declarations, return, if/while/do/for/switch, labels, jump statements, and compound statements. None of these branches matched a bare `;` token, so execution fell through to the final fallback:
```c
// expression_stmt fallback
parse_expression(...);  // unconditionally calls parse_expression
```
Since `parse_expression()` has no production starting with `;`, it immediately errored. Per C99 §6.8.3, a bare `;` is a valid null statement (an expression_statement with no expression).

## Test Plan

Two new portable integration tests will be added:
1. `test_while_null_stmt` — verifies `while(0);` followed by further statements compiles and executes correctly (confirming the program continues past the null-bodied loop).
2. `test_for_null_stmt` — verifies `for(;;);` (infinite loop with null body) compiles (not executed; serves as a syntax check) and that a bounded `for(int i = 0; i < N; i++);` correctly counts through iterations and continues past the loop.

Both tests will be ordinary integration tests (.c + .expected companion files), requiring no special compiler flags.

## Files Changed This Stage

- `src/parser.c` — function `parse_statement()`, add null-statement check before expression-statement fallback (+9 lines).
- `src/version.c` — version bump from `01750000` to `01760000`.
- `docs/grammar.md` — `<expression_statement>` production made optional.
- `docs/outlines/checklist.md` — new Stage 176 section.
- `test/integration/test_while_null_stmt/` (new: .c + .expected).
- `test/integration/test_for_null_stmt/` (new: .c + .expected).
- `docs/self-compilation-report.md` — new stage-176 entry (to be written).

## Test Results Expected

Full suite should pass with 2077 portable tests (165 unit, 1286 valid, 261 invalid, 194 integration [+2 new], 50 print-AST, 100 print-tokens, 21 print-asm), 194 system-include, 2 optional-library, 5 build-tool. Zero regressions. Self-host C0→C1→C2 cycle should complete successfully with no source changes required.
