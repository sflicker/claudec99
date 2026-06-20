# Stage 158 Kickoff — Preprocessor Comment Stripping in #if/#elif Conditions

## Summary of the spec

Stage 158 fixes a C preprocessor bug where inline comments (`/* ... */` and `//`) within `#if` and `#elif` preprocessor directives are not stripped before condition evaluation. This causes the expression parser to fail when it encounters comment syntax as if it were part of the condition expression itself.

The root cause: when zlib.h (included via `#include <zlib.h>`) is processed, it includes zconf.h which contains a line like:

```c
#if defined(__OS400__) && !defined(STDC)    /* iSeries (formerly AS/400). */
```

The condition text extraction in `src/preprocessor.c` includes the trailing comment. When `eval_cond_multiplicative()` evaluates the expression, it sees `/` as a division operator, tries to parse `*` as the right operand, and `eval_cond_primary()` fails with "error: #if requires an integer constant or defined(...)".

The fix is localized to `src/preprocessor.c` only: add a `strip_cond_comments()` helper function that removes `/* ... */` block comments and `//` line comments from condition text, and call it in the two places where condition text is extracted (`#if` handler and `#elif` handler). No changes to tokenizer, parser, AST, or codegen are needed.

## Required tokenizer changes

None.

## Required parser changes

None.

## Required AST changes

None.

## Required code generation changes

None. This is a pure preprocessor fix.

## Implementation tasks

### 1. Add comment-stripping helper to `src/preprocessor.c`

- Implement `strip_cond_comments()` function that:
  - Takes a condition text string (e.g., `"defined(__OS400__) && !defined(STDC)    /* comment */"`)
  - Scans through the string character by character
  - Replaces all `/* ... */` block comments with spaces (preserving alignment)
  - Replaces all `//` comments to end-of-line with spaces
  - Returns the cleaned condition text
  - Handles nested comment-like patterns correctly (e.g., `"string_with_/*_in_it"` where `/` is in a string literal context; for simplicity, assume `#if` conditions do not contain string literals with comment syntax)

- Call this function in the `#if` handler after extracting `cond_text`
- Call this function in the `#elif` handler after extracting `cond_text`

### 2. Update version stamp

- Change `VERSION_STAGE` in `src/version.c` from `"01570000"` to `"01580000"`

### 3. Integration tests

Create three integration tests under `test/integration/`:

1. **test_if_with_block_comment** — `#if` with inline `/* */` comment
   - Verifies `#if defined(SOMETHING) /* comment */` preprocesses correctly
   - Input: preprocessor directive with block comment
   - Expected: code after `#if` is included/excluded correctly based on the condition

2. **test_elif_with_block_comment** — `#elif` with inline `/* */` comment
   - Verifies `#elif defined(SOMETHING) /* comment */` preprocesses correctly
   - Input: chain of `#if`/`#elif` where `#elif` has block comment
   - Expected: correct branch selection

3. **test_zlib_pattern** — complex condition similar to zlib's zconf.h
   - Verifies the actual zlib pattern: `#if defined(__OS400__) && !defined(STDC)    /* iSeries */`
   - Input: similar preprocessor pattern (may use different macros if zlib unavailable)
   - Expected: preprocesses without error and produces correct output

Each test should verify that:
- The compiler produces no "error: #if requires an integer constant" error
- The resulting program compiles and runs successfully
- Output is as expected

### 4. Bootstrap validation

- No new C constructs are used; `strip_cond_comments()` uses only basic string scanning
- The function must compile under the C0 compiler
- Verify C89 compatibility: no VLAs, only `/* */` comments in the implementation itself, standard library functions

### 5. Commit

- Single commit with message referencing stage 158 and the zlib issue
- Include the Co-Authored-By trailer

## Test plan

### Integration tests

1. **test_if_with_block_comment** — inline block comment in `#if`
   - Preprocessor handling
   - Expected output: program compiles and runs

2. **test_elif_with_block_comment** — inline block comment in `#elif`
   - Preprocessor handling
   - Expected output: program compiles and runs

3. **test_zlib_pattern** — zlib-like complex condition
   - Preprocessor handling
   - Expected output: program compiles and runs

### Regression testing

- Full test suite (`./run_tests.sh`) must pass
- All existing preprocessor tests unaffected
- Stage 157 tests (peephole) continue to pass
- Smoke test: if zlib is available, verify `cc99 --sysinclude zlib_test.c -o zlib_test -lz` compiles successfully with no preprocessor errors

## Out of scope

- **Other preprocessor improvements** — tokenization, macro expansion improvements, include-path resolution
- **String literal handling in conditions** — assume `#if` conditions do not contain string literals (valid C99 preprocessor conditions do not allow strings)
- **Directive parsing changes** — only condition text evaluation is modified
- **Error messages** — existing error reporting for malformed conditions remains unchanged
