# Stage 54 Kickoff: #undef Support

## Summary

Stage 54 adds support for the `#undef NAME` preprocessor directive. When encountered, `#undef` removes a named macro from the macro table. If the macro is not defined, the directive is a no-op. Subsequent `#ifdef` / `#ifndef` directives and macro expansions reflect the post-undef table state.

## Required Tokenizer Changes

None. The preprocessor already tokenizes the `#undef` directive; no changes needed.

## Required Parser Changes

None. The preprocessor handles `#undef` directly; no parser modifications required.

## Required AST Changes

None. `#undef` is a preprocessor-only construct with no AST implications.

## Required Code Generation Changes

None. `#undef` is processed during preprocessing and does not reach code generation.

## Test Plan

Two test cases from the spec validate core behavior:

1. **test_undef_basic__42.c** — Define a macro, undef it, verify with `#ifndef` that it is gone. Expect exit code 42.

2. **test_undef_use_before__42.c** — Use a macro, then undef it, verify that `#ifdef` after the undef sees it as undefined. Expect exit code 42.

## Implementation Order

1. Preprocessor: add `macro_undef()` helper function; add `#undef NAME` directive branch in `preprocess_internal()`
2. Add test cases to validate basic undef and post-undef state

## Notes

- Minor typo in spec line 8: "remote" should be "remove" (non-blocking)
- No ambiguities identified; proceeding with implementation
