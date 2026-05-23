# stage-60-02 Transcript: Runtime Context Predefined Macros

## Summary

Stage 60-02 adds three predefined macros that provide runtime context at compile time: `__DATE__`, `__TIME__`, and `__STDC_HOSTED__`. These follow the same preprocessor-injection model as the existing standard predefined macros (`__STDC__`, `__STDC_VERSION__`, etc.) established in earlier stages.

`__DATE__` expands to a string literal of the form `"Mmm DD YYYY"` (e.g., `"May 23 2026"`) representing the compilation date. `__TIME__` expands to `"HH:MM:SS"` representing the compilation time. Both are computed once at the start of preprocessing using the C standard library `time()`/`localtime()`/`strftime()` and injected as object-like macros whose replacement text includes the surrounding double-quote characters, making them expand to valid C string literals. `__STDC_HOSTED__` expands to integer `1`, indicating a hosted implementation (programs can call libc).

No tokenizer, parser, AST, or codegen changes were required.

## Changes Made

### Step 1: Preprocessor

- Added `#include <time.h>` to `src/preprocessor.c`
- Added injection of `__STDC_HOSTED__` with replacement `"1"` alongside existing standard predefined macros in `preprocess_with_defines_and_includes()`
- Added a scoped block that calls `time(NULL)`, `localtime()`, and `strftime()` to format `__DATE__` as `"Mmm DD YYYY"` and `__TIME__` as `"HH:MM:SS"` using `%b %e %Y` and `%H:%M:%S` format strings (with embedded `"` characters in the replacement so they expand to C string literals)
- Injected `__DATE__` and `__TIME__` as object-like macros immediately after `__STDC_HOSTED__`

### Step 2: Tests

- Added `test/valid/test_predefined_macro_stdc_hosted__0.c`: returns `__STDC_HOSTED__ - 1` (0 on a hosted implementation)
- Added `test/valid/test_predefined_macro_date__0.c`: returns `strlen(__DATE__) > 0 ? 0 : 1` (verifies non-empty string expansion)
- Added `test/valid/test_predefined_macro_time__0.c`: returns `strlen(__TIME__) > 0 ? 0 : 1` (verifies non-empty string expansion)

## Final Results

Build clean (no new warnings). All 1061 tests pass (1058 existing + 3 new). No regressions.

Breakdown: 647 valid, 202 invalid, 53 integration, 39 print-AST, 99 print-tokens, 21 print-asm.

## Session Flow

1. Read spec and supporting files (implement-stage notes, generated-artifacts reference, README)
2. Reviewed existing predefined macro implementation (`__FILE__`, `__LINE__`, `__STDC__`, etc.) in `src/preprocessor.c`
3. Produced stage summary and planned changes
4. Generated kickoff artifact (haiku-stage-artifact-writer)
5. Implemented preprocessor changes: added `<time.h>`, injected three new macros
6. Built compiler — clean
7. Wrote three new valid tests
8. Ran full test suite — 1061/1061 pass
9. Generated milestone, transcript, and README artifacts
