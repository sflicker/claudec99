# Stage 55-02 Kickoff: Macro Expansion in `#if` and `#elif`

## Summary

This stage extends preprocessor conditional evaluation (`#if` and `#elif`) to:
1. Expand object-like macros that resolve to integer literals
2. Support `#if defined NAME` syntax without parentheses (complement to existing `#if defined(NAME)`)

Both features enable simpler, more conventional macro-based conditional compilation patterns.

## Spec Overview

**In scope:**
- Object-like macros expanding to single integer literals within `#if`/`#elif` conditions
- `#if defined NAME` syntax (no parentheses required)
- Both features apply to `#if` and `#elif` directives

**Out of scope:**
- Function-like macro expansion
- Arithmetic, comparison, and logical operators
- Undefined identifiers becoming 0
- Recursive macro expansion beyond simple object-like macros

## Required Changes

### Tokenizer
No changes required.

### Parser
No changes required.

### AST
No changes required.

### Code Generation
No changes required.

### Preprocessor
**File:** `src/preprocessor.c`

Extend `#if`/`#elif` condition evaluation to:
1. Recognize `defined NAME` (without parentheses) in addition to existing `defined(NAME)`
2. Recognize bare identifiers and look them up in the macro table
3. Return the macro's integer literal value if the macro is object-like and expands to an integer literal
4. Treat undefined identifiers as their current behavior (implementation detail)

## Test Plan

Five test cases covering macro expansion and defined checks in both `#if` and `#elif`:

1. **test_pp_if_macro_true__42.c**
   - `#define DEBUG 1` / `#if DEBUG` → evaluates to true, returns 42

2. **test_pp_if_defined_no_parens__42.c**
   - `#define DEBUG 0` / `#if defined DEBUG` → defined check (not value check), returns 42

3. **test_pp_elif_macro_chain__42.c**
   - `#define RED 0 / #define GREEN 0 / #define BLUE 1`
   - `#if RED / #elif GREEN / #elif BLUE` → third branch taken (BLUE=1), returns 42

4. **test_pp_elif_defined_no_parens__0.c**
   - Same macros as test 3
   - `#if defined RED / #elif defined GREEN / #elif defined BLUE` → first branch taken (RED is defined), returns 0

5. **test_pp_elif_defined_parens_multi__0.c**
   - Same macros as test 3
   - `#if defined(RED) / #elif defined(GREEN) / #elif defined(BLUE)` → first branch taken, returns 0

## Implementation Order

1. Preprocessor changes (`src/preprocessor.c`)
2. Test implementation and validation

## Notes

- Spec has a typo in test 3 comment: "stagus" instead of "status" (test runner unaffected)
- Stage 52-01 introduced `#if defined(NAME)` with parentheses; this stage adds the no-parentheses variant
