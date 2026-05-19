# Stage 55-04 Kickoff: Unary Preprocessor Operators

## Summary of the Spec

This stage adds support for unary operators (`+`, `-`, `!`) in preprocessor conditional expressions (`#if` and `#elif`). The operators are applied to the integer value after identifier/macro expansion, with the standard zero/nonzero evaluation rule applied afterwards.

Behavior:
- **Unary `-`** negates the value: `-1` → -1 (true), `-0` → 0 (false)
- **Unary `+`** is identity: `+1` → 1 (true), `+0` → 0 (false)
- **Unary `!`** applies logical NOT: `!0` → 1 (true), `!1` → 0 (false), `!-1` → 0 (false)
- **Chained unary operators** are allowed: `!-1` → !(−1) → false, `!!0` → !(!(0)) → false

Examples:
- `#if -1` → true (−1 ≠ 0)
- `#if +1` → true
- `#if -0` → false
- `#if !0` → true
- `#if !1` → false
- `#if !-1` → false
- `#if -FLAG` with FLAG=0 → true (−0 ≠ 0... actually 0 = 0 → false)
- `#if !FLAG` with FLAG=1 → false

## Required Tokenizer Changes

None. Tokenizer requires no changes. The unary operators (`+`, `-`, `!`) are already tokenized.

## Required Parser Changes

None. Parser requires no changes. The unary operators are handled only in preprocessor conditional evaluation.

## Required AST Changes

None. AST requires no changes.

## Required Code Generation Changes

None. Codegen requires no changes.

## Test Plan

### New test files for `test/valid/` suite

11 new valid test files covering:

1. `test_pp_if_not_flag_0__42.c` — `#if !FLAG` with FLAG=0 → true, main returns 42
2. `test_pp_if_minus_flag_0__1.c` — `#if -FLAG` with FLAG=0 → false, else returns 1
3. `test_pp_if_not_flag_1__1.c` — `#if !FLAG` with FLAG=1 → false, else returns 1
4. `test_pp_if_minus_flag_1__42.c` — `#if -FLAG` with FLAG=1 → true, main returns 42
5. `test_pp_if_plus_flag_1__42.c` — `#if +FLAG` with FLAG=1 → true, main returns 42
6. `test_pp_if_minus_1__42.c` — `#if -1` → true, main returns 42
7. `test_pp_if_plus_1__42.c` — `#if +1` → true, main returns 42
8. `test_pp_if_minus_0__1.c` — `#if -0` → false, else returns 1
9. `test_pp_if_not_0__42.c` — `#if !0` → true, main returns 42
10. `test_pp_if_not_1__1.c` — `#if !1` → false, else returns 1
11. `test_pp_if_not_minus_1__1.c` — `#if !-1` → false, else returns 1

### Spec issues to fix

1. Tests 4 & 5: Comments incorrectly state "expect status code 1" on the true-branch return 42 line. The correct expectation is 42.
2. Tests 6-11: End with `do #endif` (copy-paste artifact). Should be plain `#endif`.

## Implementation Focus

All changes are localized to `src/preprocessor.c`:

1. **Extract evaluation logic into two helper functions:**
   - `eval_cond_primary()` — parses defined operator, integer literals, and identifiers (returns the evaluated integer)
   - `eval_cond_expr()` — collects leading unary operators, calls `eval_cond_primary()`, applies unary operators innermost-first

2. **Update `#if` handler** (lines ~638-650)
   - Replace inline condition evaluation with call to `eval_cond_expr()`
   - Set `cond_val` to result

3. **Update `#elif` handler** (lines ~725-735)
   - Replace inline condition evaluation with call to `eval_cond_expr()`
   - Set `cond_val` to result

## Grammar Update

Update `docs/grammar.md` section for preprocessor conditionals:

```
<if-condition> ::= <pp-unary-expression>

<pp-unary-expression> ::= <pp-primary>
                         | <pp-unary-op> <pp-unary-expression>

<pp-unary-op> ::= "!" | "-" | "+"

<pp-primary> ::= <integer-literal>
               | "defined" "(" <identifier> ")"
               | "defined" <identifier>
               | <object-like-macro-identifier>
```

## Notes

- The unary operators are right-associative: `!-1` is parsed as `!(−1)`, not `(!(−)) 1`
- Operators are applied innermost-first when multiple are chained
- The implementation is straightforward: parse operators into a list/stack, evaluate the primary, then apply operators from the end of the list backwards (innermost-first)
