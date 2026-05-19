# Stage 55-08 Kickoff: Arithmetic Operators

## Spec Summary

Stage 55-08 adds support for arithmetic binary operators (`+`, `-`, `*`, `/`, `%`) to preprocessor conditional expressions (`#if`/`#elif`).

**Key behaviors:**
- `#if A + B`, `#if A - B`, `#if A * B`, `#if A / B`, `#if A % B`
- Standard C precedence: `*`, `/`, `%` bind tighter than `+`, `-`
- Division by zero and modulo by zero are errors
- Results follow integer arithmetic semantics

**Precedence (lowest to highest):**
- Primary/unary (e.g., `defined()`, literals, parenthesized expressions)
- Multiplicative: `*`, `/`, `%`
- Additive: `+`, `-`
- Relational: `<`, `<=`, `>`, `>=`
- Equality: `==`, `!=`
- Logical AND: `&&`
- Logical OR: `||`

**Example precedence:** `#if C + A * B == 42` evaluates as `#if C + (A * B) == 42`

**Out of scope:** bitwise operators, shifts, ternary `?:`, character constants in preprocessor expressions

## In-Scope Test Cases

1. **Addition with macros:**
   ```c
   #define A 20
   #define B 22
   #if A + B == 42
   int main() { return 42; }  // expect 42
   ```

2. **Modulo with false branch:**
   ```c
   #define X 10
   #if X % 2
   int main() { return 42; }
   #else
   int main() { return 1; }  // expect 1
   #endif
   ```

3. **Modulo with equality:**
   ```c
   #define X 10
   #if X % 2 == 0
   int main() { return 42; }  // expect 42
   #else
   int main() { return 1; }
   #endif
   ```

4. **Multiplication precedence:**
   ```c
   #define A 10
   #define B 4
   #define C 2
   #if C + A * B == 42
   int main() { return 42; }  // expect 42
   #else
   int main() { return 1; }
   #endif
   ```

**Note:** Spec test 1 contains a typo: `#if A + b == 42` uses lowercase `b`, but the macro is `#define B 22` (uppercase). Undefined identifier `b` evaluates to 0, making the expression 20+0=20, which does NOT equal 42. Implementation will use uppercase `B` so the test exercises the intended addition behavior.

## Error Cases

- Division by zero: `#if 1 / 0` should produce a compile error
- Modulo by zero: `#if 1 % 0` should produce a compile error

## Required Changes

### Tokenizer
No changes — arithmetic operators are already correctly lexed by the main lexer.

### Parser / AST / Codegen
No changes — this is purely a preprocessor change affecting `#if`/`#elif` condition evaluation.

### Preprocessor expression evaluator (`src/preprocessor.c`)

Add two new evaluator functions to the recursive-descent chain:

- **`eval_cond_additive()`** — parses and evaluates `+` and `-` expressions
  - Calls `eval_cond_multiplicative()` for operands
  - Returns the sum or difference
  - Integration point: called by `eval_cond_relational()` instead of `eval_cond_unary()`

- **`eval_cond_multiplicative()`** — parses and evaluates `*`, `/`, and `%` expressions
  - Calls `eval_cond_unary()` for operands
  - Returns the product, quotient, or remainder
  - Division by zero triggers error: `"division by zero in #if expression"`
  - Modulo by zero triggers error: `"modulo by zero in #if expression"`

Current chain: `eval_cond_relational()` → `eval_cond_unary()`
New chain: `eval_cond_relational()` → `eval_cond_additive()` → `eval_cond_multiplicative()` → `eval_cond_unary()`

### Grammar (`docs/grammar.md`)

Add preprocessor arithmetic expression rules:

- Add `<pp-additive-expression>` rule for `+` and `-`
- Add `<pp-multiplicative-expression>` rule for `*`, `/`, and `%`
- Update `<pp-relational-expression>` to reference `<pp-additive-expression>` (instead of current operand)
- Update `<pp-primary>` parenthesized form to reference the appropriate expression level

### Tests

- 4 valid test files (one per test case above, with typo corrected in test 1)
- 2 invalid test files for division-by-zero and modulo-by-zero errors

## Implementation Order

1. Add `eval_cond_multiplicative()` and `eval_cond_additive()` functions to `src/preprocessor.c`
2. Update `eval_cond_relational()` to call `eval_cond_additive()` instead of `eval_cond_unary()`
3. Add 4 valid spec test files in `test/spec/` (use corrected uppercase `B` in test 1)
4. Add 2 invalid test files in `test/invalid/` for zero-division error cases
5. Update `docs/grammar.md`
6. Update `README.md`
7. Run full test suite

## Decisions

- **No short-circuit evaluation:** Both operands are always evaluated (consistent with existing evaluator style)
- **Spec typo correction:** Test 1 will use `A + B` instead of `A + b` so the test exercises the intended addition behavior and produces the expected exit code 42
- **Error handling:** Division by zero and modulo by zero each produce distinct error messages for clarity
