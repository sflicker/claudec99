# Stage 55-05 Kickoff: Parenthesized Expression Support for `#if`/`#elif` Preprocessor Conditions

## Spec Summary

Stage 55-05 adds parenthesized expression support to preprocessor conditional expressions in `#if` and `#elif` directives. This allows constructs like `#if (FLAG)`, `#if (1)`, `#if ((VALUE))`, `#if (defined(DEBUG))`, and combinations with existing unary operators.

**In scope:**
- `(expr)` — single parenthesized expressions
- nested parentheses, e.g. `(((1)))`
- parenthesized integer literals: `(1)`, `(0)`
- parenthesized identifiers/macros: `(FLAG)`, `(VALUE)`
- parenthesized `defined(...)`: `(defined(DEBUG))`, `(defined NAME)`
- combinations with unary operators: `(!VALUE)`, `(-VALUE)`, `(!(defined(X)))`

**Out of scope:**
- binary arithmetic in conditions
- comparison operators in conditions
- logical `&&` and `||` in conditions

## Tokenizer Changes

None required. The `(` and `)` tokens already exist and are recognized by the lexer.

## Parser Changes

None required. The preprocessor condition expression parser is separate from the main C parser and does not interact with the main statement/expression parsers.

## AST Changes

None required. The preprocessor evaluates conditions at macro-expansion time, producing compile-time integer values (0 = false, nonzero = true). No AST nodes are created for preprocessor expressions.

## Code Generation Changes

None required. Preprocessor directives are fully resolved before code generation begins.

## Implementation Plan

1. **Preprocessor (src/preprocessor.c)**: Extend `eval_cond_primary()` to handle `(` tokens.
   - When `(` is encountered, recursively call `eval_cond_expr()` to evaluate the nested expression.
   - Consume the matching `)` or error if missing.
   - Return the result of the nested expression.

2. **Tests (test/valid/)**: Add 8 new test files corresponding to the spec test cases:
   - Parenthesized macro
   - `defined()` with unary `!` (confirming existing behavior)
   - Parenthesized literal
   - Parenthesized in `#elif`
   - Unary operator in parentheses with `#elif`
   - Negation in parentheses (nonzero first branch)
   - Nested parentheses
   - `defined()` in parentheses

3. **Documentation (docs/grammar.md)**: Update the preprocessor expression grammar to reflect parenthesized primary expressions.

4. **README.md**: Update the stage line and increment test totals (currently 588 valid, add 8 → 596 valid; total 966 → 974).

## Known Ambiguities and Decisions

**Spec Issue Correction:** Test 3 in the spec has malformed C code (`else` instead of `#else`, missing `#endif`). The test files will use corrected versions.

**Nested Parentheses:** The recursive call to `eval_cond_expr()` naturally supports arbitrary nesting depth.

**Error Handling:** Missing closing `)` will be caught when `eval_cond_primary()` attempts to consume the expected `)` token.

## Implementation Order

1. Update src/preprocessor.c with parenthesis handling in eval_cond_primary()
2. Create test files in test/valid/
3. Verify all tests pass
4. Update docs/grammar.md
5. Update README.md
