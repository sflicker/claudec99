# Stage 55-07 Kickoff: Logical Preprocessor Operators

## Spec Summary

Stage 55-07 adds support for logical `&&` and `||` operators to preprocessor conditional expressions (`#if`/`#elif`).

**Key behaviors:**
- `#if A && B`, `#if A || B`, and combinations with `defined()`, equality, and relational operators
- Truthiness: 0 = false, nonzero = true
- Result: `&&` and `||` expressions evaluate to 0 or 1
- Precedence (lowest to highest):
  - Primary/unary (e.g., `defined()`, literals, parenthesized expressions)
  - Equality (`==`, `!=`)
  - Relational (`<`, `<=`, `>`, `>=`)
  - Logical AND (`&&`)
  - Logical OR (`||`)

**Example precedence:** `#if A || B && C` evaluates as `#if A || (B && C)`

**Out of scope:** arithmetic binary operators, bitwise operators, shifts, ternary `?:`

## In-Scope Test Cases

1. **Logical AND with defined macros:**
   ```c
   #define A 1
   #define C 1
   #if A && C
   int main() { return 42; }  // expect 42
   ```

2. **Logical OR with precedence (AND binds tighter):**
   ```c
   #define A 0
   #define B 1
   #define C 0
   #if A || B && C
   int main() { return 42; }  // expect 42 (A || (B && C) = 0 || 0 = 0, false)
   ```

3. **Multi-branch with equality and AND:**
   ```c
   #define PRIORITY 1
   #define SEVERITY 2
   #if PRIORITY == 1 && SEVERITY == 1
   int main() { return 1; }
   #elif PRIORITY == 1 && SEVERITY == 2
   int main() { return 42; }  // expect 42
   ```

**Note:** Spec test 3 contains a typo: `#define SERVERITY 2` should be `#define SEVERITY 2`. Implementation will use the corrected spelling so the test exercises the intended `#elif` branch.

## Required Changes

### Tokenizer
No changes — `&&` and `||` are already correctly lexed by the main lexer. The preprocessor evaluator reads the condition string directly without full tokenization.

### Parser / AST / Codegen
No changes — this is purely a preprocessor change affecting `#if`/`#elif` condition evaluation.

### Preprocessor expression evaluator (`src/preprocessor.c`)

Add two new evaluator functions to the recursive-descent chain:

- **`eval_cond_logical_and()`** — parses and evaluates `&&` expressions
  - Calls `eval_cond_equality()` for left and right operands
  - Returns 1 if both sides are nonzero, 0 otherwise

- **`eval_cond_logical_or()`** — parses and evaluates `||` expressions
  - Calls `eval_cond_logical_and()` for left and right operands
  - Returns 1 if either side is nonzero, 0 otherwise

- **Update `eval_cond_expr()`** — modify the top-level entry point to delegate to `eval_cond_logical_or()` instead of directly calling `eval_cond_equality()`

Current chain: `eval_cond_expr()` → `eval_cond_equality()`
New chain: `eval_cond_expr()` → `eval_cond_logical_or()` → `eval_cond_logical_and()` → `eval_cond_equality()`

### Grammar (`docs/grammar.md`)

Update preprocessor conditional expression rules to include logical operators:

- Update `<if-condition>` to reference `<pp-logical-or-expression>` (instead of current entry point)
- Add `<pp-logical-or-expression>` rule
- Add `<pp-logical-and-expression>` rule
- Update `<pp-primary>` parenthesized form to reference `<pp-logical-or-expression>` (allowing full expressions inside parentheses)

### Tests

- 3 spec test files (one per test case above)
- Additional coverage in `test/valid/` as needed

## Implementation Order

1. Add `eval_cond_logical_and()` and `eval_cond_logical_or()` functions to `src/preprocessor.c`
2. Update `eval_cond_expr()` to call `eval_cond_logical_or()`
3. Add 3 spec test files in `test/spec/` (use corrected SEVERITY spelling in test 3)
4. Update `docs/grammar.md`
5. Update `README.md`
6. Run full test suite

## Decisions

- **No short-circuit evaluation:** Both operands are always evaluated. This simplifies the implementation and aligns with the existing evaluator style (which doesn't optimize for constant expressions).
- **Spec typo correction:** Test 3 will use `SEVERITY` instead of `SERVERITY` so the test exercises the intended `#elif` branch and produces the expected exit code 42.
