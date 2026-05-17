# stage-52-03 Kickoff

## Spec Summary

Add support for `#if <integer-constant>` in the preprocessor. This stage implements basic conditional preprocessing based on the truth value of integer constant expressions.

Key rules:
- `0` evaluates to false; nonzero evaluates to true
- Whitespace allowed between `#` and `if`, and between `if` and the integer
- `#if0` (no space) is NOT a valid directive
- Only a single integer literal is valid — no expressions, identifiers, or `defined()`
- Parenthesized forms like `#if(1)` are out of scope

## Derived Stage Values

| Component | Change Required |
|-----------|-----------------|
| Tokenizer | None |
| Parser | None |
| AST | None |
| Code generation | None |
| Preprocessor | Yes — add `#if` handler |

## Planned Changes

### Preprocessor (`src/preprocessor.c`)
- Add new `#if` directive handler to dispatch after existing `#ifdef`/`#ifndef` handlers
- Parse integer constant following the `#if` keyword (with optional whitespace)
- Evaluate as true if nonzero, false if zero
- Track conditional state in the existing conditional stack
- Reject directives missing an integer literal or containing expressions/identifiers

## Test Plan

**Valid tests (3):**
1. `test/valid/test_pp_if_true_branch__42.c` — `#if 1` true branch → return 42
2. `test/valid/test_pp_if_false_branch__1.c` — `#if 0` false branch via else → return 1
3. `test/valid/test_pp_if_nonzero__42.c` — `#if 113` nonzero → return 42

**Invalid tests (3):**
1. `test/invalid/test_pp_if_missing_integer__integer_constant.c` — `#if` with no integer
2. `test/invalid/test_pp_if_identifier__integer_constant.c` — `#if X`
3. `test/invalid/test_pp_if_expression__extra_tokens.c` — `#if 1 + 4`

## Implementation Order

1. Preprocessor handler — add `#if` directive to `src/preprocessor.c`
2. Valid tests (3)
3. Invalid tests (3)
4. Update `docs/grammar.md` to document `#if` syntax
5. Update `README.md` to mention stage-52-03

## Ambiguities and Decisions

None identified. The spec is clear on what is in scope and what is not.

## Notes

- Spec has minor typo on line 47: `` ` comparisons expressions `` should be `- comparison expressions` (stray backtick)
- The preprocessor's conditional stack will reuse existing infrastructure from `#ifdef`/`#ifndef` handling
