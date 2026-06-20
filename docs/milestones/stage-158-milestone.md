# Stage 158 - Compile Failure with External Library

## Summary

Stage 158 fixes two preprocessor bugs that prevented compilation of programs using external libraries like zlib. Both bugs involved comment handling in the preprocessor:

**Bug 1 (primary)**: `/* ... */` comments embedded inside `#if`/`#elif` condition text were passed verbatim to the expression evaluator. `/*` was misinterpreted as the division operator, causing `eval_cond_primary` to fail with "error: #if requires an integer constant or defined(...)". Root cause: zconf.h line 227 has `#if defined(__OS400__) && !defined(STDC)    /* iSeries (formerly AS/400). */`.

**Bug 2**: `#` characters inside `/* ... */` comments within macro replacement text (e.g., `#define ARG_MAX 131072 /* # bytes ... */`) triggered "error: hash in object like macro is not permitted" because the replacement validation loop did not skip comment spans.

Both bugs made it impossible to `#include` real system headers from zlib, glibc, and other external libraries that use inline comments in preprocessor directives and macro definitions.

## Changes Made

### Preprocessor Comment Handling

- Added `strip_cond_comments()` helper in `src/preprocessor.c`: replaces `/* */` and `//` comments with spaces in condition expression text. Applied in both `#if` and `#elif` handlers before passing the condition to `eval_cond_expr()`.
- Extended the `#define` replacement validation loop in `src/preprocessor.c` to skip over `/* */` comment spans, preventing false "hash in macro" errors when `#` appears inside a comment.

### Version Update

- `src/version.c`: incremented version to `01580000` (Stage 158).

### Integration Tests

Three new test suites verify correct preprocessor behavior:

- `test/integration/test_if_inline_comment/`: `#if` with trailing comment (zconf.h pattern)
- `test/integration/test_elif_inline_comment/`: `#elif` with trailing comment
- `test/integration/test_macro_comment_hash/`: macro definition with `#` inside a comment

All three tests verify that comments are stripped without affecting expression evaluation and compilation succeeds.

## Final Results

All 2056 portable tests pass (165 unit, 1286 valid, 261 invalid, 173 integration, 50 print-AST, 100 print-tokens, 21 print-asm). Self-host C0→C1→C2 verified with all tests passing at every stage. The compiler can now successfully compile programs using external libraries with inline comments in preprocessor directives.
