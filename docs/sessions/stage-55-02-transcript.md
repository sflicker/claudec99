# stage-55-02 Transcript: Macro Expansion in `#if` and `#elif`

## Summary

Stage 55-02 extends preprocessor conditional evaluation to accept two new forms of conditions in `#if` and `#elif`:

1. **Object-like macro identifiers**: When an identifier appears in an `#if` or `#elif` condition, the preprocessor now looks it up in the macro table. If the macro is object-like (param_count == -1), its replacement list is parsed as an integer literal and used as the condition value. If the identifier is not a defined macro, an error is raised.

2. **`defined NAME` syntax**: The `defined` operator now accepts both parenthesized (`defined(NAME)`) and non-parenthesized (`defined NAME`) forms. When the non-parenthesized form is detected (by checking if the next non-whitespace character after `defined` is not `(`), the parser reads the identifier directly and performs a macro lookup, returning 1 if defined, 0 if not.

Both features were added to the `#if` and `#elif` condition evaluators in `src/preprocessor.c` under identical logic. The spec contained one typo in test 3 ("stagus" instead of "status"), which was corrected in the generated test file.

## Changes Made

### Step 1: Preprocessor Condition Evaluation

- Extended `evaluate_if_condition()` in `src/preprocessor.c` to handle three cases:
  1. Integer literals (existing behavior)
  2. `defined(NAME)` or `defined NAME` operator expressions
  3. Object-like macro identifiers that expand to integer literals
- Extended `evaluate_elif_condition()` with identical logic to `evaluate_if_condition()`
- Added macro table lookup and expansion: if the condition is an identifier, look it up; if found and object-like, parse its replacement as an integer; if not found, emit an error.
- Added support for `defined NAME` (no parens) by detecting that the next non-whitespace character after `defined` is not `(`.

### Step 2: Test Suite

- Created 5 new valid tests:
  - `test_pp_if_macro_true__42.c`: Simple `#define ENABLED 1` with `#if ENABLED`
  - `test_pp_if_defined_no_parens__42.c`: `#if defined NAME` without parentheses
  - `test_pp_elif_macro_chain__42.c`: Chain of `#elif MACRO` conditions with multiple macros
  - `test_pp_elif_defined_no_parens__0.c`: `#elif defined NAME` without parentheses
  - `test_pp_elif_defined_parens_multi__0.c`: Multiple `#elif defined(NAME)` branches
- Renamed 1 invalid test:
  - `test_pp_if_identifier__integer_constant.c` → `test_pp_if_identifier__identifier_is_not_a_defined.c` to match the updated error message for undefined identifiers in `#if`.

### Step 3: Grammar Documentation

- Updated `docs/grammar.md` section on `<if_constant_directive>` and `<elif_constant_directive>` to document the new `if-condition` production:
  ```
  <if-condition> ::= <integer-literal>
                   | "defined" "(" <identifier> ")"
                   | "defined" <identifier>
                   | <object-like-macro-identifier>
  ```

### Step 4: README Documentation

- Updated README.md "Through stage 55-01" to "Through stage 55-02"
- Updated Preprocessor section `#if`/`#elif` documentation to mention:
  - Object-like macro identifiers now supported as conditions
  - `defined NAME` (without parens) now accepted alongside `defined(NAME)`
- Updated "Not yet supported" section to reflect that object-like macros and `defined NAME` are now supported
- Updated test totals from "948 total" and "569 valid" to "953 total" and "574 valid"

## Final Results

All 953 tests pass:
- Valid: 574 (5 new tests added)
- Invalid: 193 (1 renamed)
- Integration: 27
- Print-AST: 39
- Print-tokens: 99
- Print-asm: 21

No regressions. Full test suite clean.

## Session Flow

1. Read spec and supporting templates
2. Reviewed existing preprocessor condition evaluation logic
3. Analyzed the spec requirements for macro expansion and `defined NAME` syntax
4. Extended condition evaluators in `src/preprocessor.c` for both features
5. Generated and reviewed test files for coverage of all cases
6. Ran full test suite and confirmed all 953 tests pass
7. Updated grammar documentation with new `if-condition` production
8. Updated README.md to document new features and final test counts
9. Generated milestone and transcript artifacts

## Notes

- The spec test 3 had a typo: "stagus" instead of "status". This was fixed in the generated test file.
- Macro expansion in conditions only supports object-like macros (not function-like). Function-like macro expansion in `#if` conditions is out of scope and will be addressed in a later stage if needed.
- The implementation correctly rejects undefined identifiers in `#if`/`#elif` conditions with an appropriate diagnostic error.
