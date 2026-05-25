# stage-70-03 Transcript: Add Line/Column to Errors and Warnings

## Summary

Stage 70.03 enhances compiler diagnostics by prefixing error and warning messages with source location information (`<file>:<line>:<col>:`). The line and column numbers are obtained from the Token instances that carry this metadata (added in stage 70.02). Parser errors—the most common class—now report precise source positions. Codegen errors and warnings continue without position information as a known limitation, since AST nodes do not carry token references.

The implementation follows a systematic approach: new functions `compile_error_at` and `compile_warning_at` accept position parameters, and a `PARSER_ERROR` macro in the parser centralizes all parser-level error calls to supply Token position data automatically.

## Changes Made

### Step 1: Error/Warning API Enhancements in util.h and util.c

- Added `extern int g_warnings_are_errors` global declaration and definition (initialized to 0).
- Added `compile_error_at(const char *file, int line, int col, const char *fmt, ...)` function (noreturn), which prints `file:line:col: ` prefix when file is non-NULL and line > 0, then delegates to error reporting.
- Added `compile_warning_at(const char *file, int line, int col, const char *fmt, ...)` function, which prints optional position prefix and either calls error reporting or `vfprintf` to stderr based on `g_warnings_are_errors`.
- Refactored `compile_error` to use internal `do_compile_error(const char *fmt, va_list ap)` helper (marked noreturn) to avoid duplication between `compile_error` and `compile_error_at`.

### Step 2: Parser Integration

- Added `PARSER_ERROR(parser, ...)` macro at the top of src/parser.c; the macro calls `compile_error_at(parser->current.file, parser->current.line, parser->current.col, ...)`.
- Replaced all 107 instances of `compile_error(` with `PARSER_ERROR(parser, )` throughout src/parser.c to automatically inject source position from the current Token.

### Step 3: Codegen Warning Integration

- Updated `codegen_warn_const_discard` in src/codegen.c to call `compile_warning_at(NULL, 0, 0, ...)` instead of performing a local check against `cg->warnings_are_errors` followed by `fprintf`.
- Removed the local `cg->warnings_are_errors` field and logic, relying instead on the global `g_warnings_are_errors` flag set during compiler initialization.

### Step 4: Compiler Flag Integration

- Updated src/compiler.c to set `g_warnings_are_errors = 1` when the `-Werror` flag is parsed, enabling the global switch used by both parser and codegen warning functions.

### Step 5: Testing and Verification

- Ran full test suite: all 1143 tests pass (705 valid, 212 invalid, 67 integration, 39 print-AST, 99 print-tokens, 21 print-asm).
- Invalid tests continue to work because they use substring matching via grep with `-qi` (case-insensitive substring search); the presence of a `file:line:col:` prefix does not break test assertions.

## Final Results

All 1143 tests pass with no regressions. Build succeeds cleanly. Parser errors now report source file, line, and column information in the standard compiler format. Warning behavior with `-Werror` is consistent with the global flag.

## Session Flow

1. Read stage 70-03 spec and reviewed stage 70.02 Token implementation.
2. Analyzed current error/warning infrastructure in util.c and identified refactoring needs.
3. Designed position-aware error API (`compile_error_at` and `compile_warning_at`) and created helper functions.
4. Added `PARSER_ERROR` macro to centralize parser error calls with automatic Token position injection.
5. Systematically replaced all 107 `compile_error` calls in parser.c with `PARSER_ERROR(parser, )`.
6. Updated codegen warning function and compiler flag integration for `-Werror`.
7. Built compiler and ran full test suite to verify no regressions.
8. Confirmed all test categories pass and invalid tests continue to work via substring matching.
9. Generated milestone summary, transcript, and README update artifacts.
