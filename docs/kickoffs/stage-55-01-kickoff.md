# Stage 55-01 Kickoff: `defined` operator in `#if` and `#elif`

## Spec Summary

Add support for the `defined(NAME)` operator in preprocessor conditional expressions (`#if` and `#elif`).

**Rules:**
- `defined(NAME)` evaluates to 1 if NAME is currently defined as a macro
- `defined(NAME)` evaluates to 0 if NAME is not currently defined

**Out of Scope:** macro expansion in #if expressions, `&&`/`||`/`!`, comparison operators, arithmetic expressions, treating arbitrary identifiers as 0.

## Derived Stage Values

| Component | Value |
|-----------|-------|
| Stage Label | `stage-55-01` |
| Stage Display | `Stage 55-01` |
| Complexity | Low |
| Primary System | Preprocessor |
| Estimated Duration | 1-2 hours |

## Required Changes

### Tokenizer
No changes required.

### Parser
No changes required.

### AST
No changes required.

### Code Generation
No changes required.

### Preprocessor (`src/preprocessor.c`)
Extend the `#if` and `#elif` handlers in `preprocess_internal()` to:
1. Recognize the `defined(NAME)` syntax before the existing integer-constant parsing
2. Parse `defined(NAME)` as a function-like construct where NAME is a macro identifier
3. Look up NAME in the macro table
4. Set `cond_val` to 1 if NAME is defined, 0 if not

## Test Plan

Three new test files in `test/valid/`:

1. **`test_pp_if_defined_true__42.c`**
   - Define a macro `DEBUG`
   - Use `#if defined(DEBUG)` → expect true branch to compile
   - Return 42
   - Validates basic `defined()` with a defined macro

2. **`test_pp_if_defined_after_undef__1.c`**
   - Define `DEBUG`, then `#undef DEBUG`
   - Use `#if defined(DEBUG)` → expect false branch to compile
   - Return 1
   - Validates that `defined()` correctly tracks macro state after undef

3. **`test_pp_elif_defined__42.c`**
   - Define `FIRST`, leave `SECOND` undefined
   - Use `#if defined(SECOND)` false, `#elif defined(FIRST)` true
   - Return 42 from the elif branch
   - Validates `defined()` works in `#elif` directives

## Implementation Order

1. Identify where `#if`/`#elif` conditionals are currently parsed in `preprocess_internal()`
2. Implement `defined(NAME)` parsing before the existing integer-constant path
3. Add macro lookup logic
4. Create the three test cases
5. Run all tests to verify correctness

## Ambiguities / Decisions

None. The spec is clear and unambiguous. Only the parenthesized form `defined(NAME)` is required.
