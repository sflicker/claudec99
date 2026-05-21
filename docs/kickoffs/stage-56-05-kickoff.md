# Stage 56-05: Standard Predefined Macros

## Spec Summary

Add three predefined macros that are automatically defined and inserted into the macro table before preprocessing starts:

```c
#define __STDC__ 1
#define __STDC_VERSION__ 199901
#define __CLAUDEC99__ 1
```

These macros behave like ordinary object-like macros and are available in `#if` directives, `#ifdef` conditionals, and normal source code macro expansion. User `-D` definitions should be rejected if they attempt incompatible redefinitions. Predefined macros can be undefined with `#undef`.

### Out of Scope
- `__DATE__` and `__TIME__`
- `__GNUC__` and `__GNUC_MINOR__`
- Architecture/platform macros
- Exact GCC/Clang compatibility

## Tokenizer Changes

None required.

## Parser Changes

None required.

## AST Changes

None required.

## Code Generation Changes

None required.

## Preprocessor Changes

In `src/preprocessor.c`, update `preprocess_with_defines_and_includes()` to call `macro_define()` three times before the user `-D` define loop to insert the predefined macros into the macro table:

```c
macro_define(&macro_table, "__STDC__", "1");
macro_define(&macro_table, "__STDC_VERSION__", "199901");
macro_define(&macro_table, "__CLAUDEC99__", "1");
```

This ensures the macros are available for all subsequent preprocessing steps including `#if` conditionals.

## Test Plan

Create integration tests in `test/integration/`:

1. **`test_pp_predef_stdc_version_if__42.c`** — Tests `#if __STDC__ == 1 && __STDC_VERSION__ >= 199901` evaluates true. Expected return: 42.

2. **`test_pp_predef_claudec99_ifdef__42.c`** — Tests `#ifdef __CLAUDEC99__` evaluates true. Expected return: 42.

3. **`test_pp_predef_stdc_undef__1.c`** — Tests that `#undef __STDC__` makes the macro unavailable, and subsequent expansion is prevented. Expected return: 1.

4. **`test_pp_predef_redefine_incompatible.c`** (invalid) — Tests that attempting `#define __STDC__ 0` is rejected as an incompatible redefinition. Expected to fail validation.

## Implementation Order

1. Implement predefined macro definitions in `src/preprocessor.c` in `preprocess_with_defines_and_includes()`
2. Create integration test directory structures
3. Verify tests pass

## Ambiguities & Notes

- Spec test has typos (three underscores `___STDC__` and missing `#endif`); implementation uses correct `__STDC__` with two underscores
- Predefined macros must be inserted before user `-D` definitions to allow user code to conditionally `#undef` them
- Incompatible redefinition detection relies on existing macro conflict detection logic
