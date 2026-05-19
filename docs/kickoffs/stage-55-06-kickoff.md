# Stage 55-06 Kickoff: Equality and Relational Operators for `#if`/`#elif` Preprocessor Conditions

## Spec Summary

Stage 55-06 adds binary equality and relational operators to preprocessor conditional expressions in `#if` and `#elif` directives. This allows comparisons like `#if VERSION >= 2`, `#elif VALUE != 0`, and `#if VALUE < 10`.

**In scope:**
- Equality operators: `==`, `!=`
- Relational operators: `<`, `<=`, `>`, `>=`
- Comparisons with integer literals and macro-expanded identifiers
- Left-associative evaluation of comparison chains
- Combinations with existing unary operators and parenthesized expressions

**Out of scope:**
- Logical operators `&&` and `||`
- Arithmetic operators (`+`, `-`, `*`, `/`, `%`)
- Bitwise operators (`&`, `|`, `^`, `~`, `<<`, `>>`)

## Tokenizer Changes

None required. The comparison token sequences are recognized by the lexer (e.g., `<`, `>`, `=`, `!`). The preprocessor condition evaluator operates on text tokens, not on the main C lexer.

## Parser Changes

None required. The preprocessor condition expression parser is separate from the main C parser and does not interact with the main statement/expression parsers.

## AST Changes

None required. The preprocessor evaluates conditions at macro-expansion time, producing compile-time integer values (0 = false, nonzero = true). No AST nodes are created for preprocessor expressions.

## Code Generation Changes

None required. Preprocessor directives are fully resolved before code generation begins.

## Implementation Plan

1. **Preprocessor (src/preprocessor.c)**: Extend the condition expression evaluator with new precedence levels.
   - Add `eval_cond_unary()` — extract unary operator handling from `eval_cond_expr()` into its own level
   - Add `eval_cond_relational()` — call `eval_cond_unary()`, then handle `<`, `<=`, `>`, `>=` left-associatively
   - Add `eval_cond_equality()` — call `eval_cond_relational()`, then handle `==`, `!=` left-associatively
   - Update `eval_cond_expr()` to delegate to `eval_cond_equality()`
   - `eval_cond_primary()` continues to call `eval_cond_expr()` for parenthesized sub-expressions (correct precedence)

2. **Tests (test/valid/)**: Add 7 new test files corresponding to the spec tests and additional coverage:
   - `test_pp_if_ge_macro__42.c` — `#if VERSION >= 2` (true, returns 42)
   - `test_pp_if_ne_macro__42.c` — `#if VALUE != 0` (false, else branch returns 42)
   - `test_pp_elif_gt_ge_macro__42.c` — `#if VERSION > 8` (false), `#elif VERSION >= 6` (true, returns 42)
   - `test_pp_if_eq_macro__42.c` — equality operator `==`
   - `test_pp_if_lt_macro__42.c` — less-than operator `<`
   - `test_pp_if_le_macro__42.c` — less-than-or-equal operator `<=`
   - `test_pp_if_gt_macro__42.c` — greater-than operator `>`

3. **Documentation (docs/grammar.md)**: Update the preprocessor expression grammar to add relational and equality precedence levels.

4. **README.md**: Update the stage line and increment test totals (currently 596 valid, add 7 → 603 valid; total 974 → 981).

## Known Ambiguities and Decisions

**Operator Precedence:** Equality operators (`==`, `!=`) have lower precedence than relational operators (`<`, `<=`, `>`, `>=`), which have lower precedence than unary operators. This follows standard C precedence and ensures expressions like `VALUE > 0 == 1` parse as `(VALUE > 0) == 1`.

**Left Associativity:** All comparison operators associate left-to-right, so `A > B > C` evaluates as `(A > B) > C`. This is consistent with C semantics.

**Integer Comparison:** All operands must be integers after macro expansion and unary operator application. Non-integer results (e.g., from `defined()` outside relational context) will be type errors.

## Implementation Order

1. Update src/preprocessor.c with relational and equality operator handling
2. Create test files in test/valid/
3. Verify all tests pass
4. Update docs/grammar.md
5. Update README.md
