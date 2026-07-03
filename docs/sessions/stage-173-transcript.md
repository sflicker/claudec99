# stage-173 Transcript: GNU Named Variadic Macro Parameters

## Summary

A user reported a compiler failure when trying to compile a networking program with `cc99 -c net_helpers.c` using real system headers (`--sysinclude`). The error was `expected ',' or ')' in macro parameter list`. Investigation revealed that without `--sysinclude`, the error is expected (cc99 uses stub headers by default), but with the system headers enabled, a real bug surfaced: `/usr/include/linux/stddef.h` defines `__struct_group(TAG, NAME, ATTRS, MEMBERS...)` using the GNU named variadic macro parameter extension (an identifier immediately followed by `...` instead of bare `...`). This header is pulled in transitively by sys/socket.h, netinet/in.h, and netdb.h, blocking all networking headers. The fix is preprocessor-only: extend the macro definition parser to recognize and store named variadic parameters, and update substitution sites to use the actual variadic name instead of hardcoded `"__VA_ARGS__"`.

## Changes Made

### Step 1: Investigation and Diagnosis

- Reproduced the reported error: `cc99 -c net_helpers.c` fails with include-file-not-found without `--sysinclude` (expected; cc99's default include path is test/include stubs only).
- With `--sysinclude` (correct invocation), the real error surfaced: `error: expected ',' or ')' in macro parameter list` at some point in the system header include chain.
- Performed header bisection by selectively including networking headers (sys/socket.h, netinet/in.h, netdb.h) to isolate the problem.
- Added temporary debug instrumentation to the preprocessor to print macro definitions being parsed.
- Root cause identified: `/usr/include/linux/stddef.h` defines `__struct_group(TAG, NAME, ATTRS, MEMBERS...)` with a GNU extension—the final parameter has no preceding comma, only `...` suffix. This is syntax the cc99 preprocessor did not yet support.

### Step 2: Preprocessor Changes

- Modified `MacroDef` struct in `src/preprocessor.c` to add `char *va_name` field: holds the variadic parameter's substitution name (`"__VA_ARGS__"` for the two standard forms `(...)` and `(a, b, ...)`, or the user-given identifier for the GNU named form `(a, b, args...)`). NULL when the macro is not variadic.
- Updated `#define` parameter-list parser to recognize `NAME...` (identifier immediately followed by `...`, no preceding comma required) as the final variadic parameter. When parsing each parameter name, the parser now peeks ahead: if `...` immediately follows, it treats the identifier as the va-name (not a fixed parameter), requires `)` next, and sets the variadic flag. Error handling mirrors the existing anonymous-`...` validation.
- Modified `macro_define()` function signature to add `va_name` parameter (owned, nullable). The function stores the given name or defaults to `strdup("__VA_ARGS__")` for the standard forms. Proper cleanup on redefinition, `#undef`, and table teardown.
- Redefinition compatibility check extended to also compare va_name presence and value.
- Updated the two macro-expansion substitution sites (in `expand_macros_text()` and `preprocess_internal()`) that were hardcoding `ext_params[def->param_count] = "__VA_ARGS__"` to instead use `def->va_name`.
- Updated the `#`/`##` replacement-text validator's `__VA_ARGS__` special case to check against the macro's actual effective va_name instead of a hardcoded string.
- All 30 internal `macro_define()` call sites for predefined macros (__STDC__, __x86_64__, etc.) updated to pass NULL for the new parameter (all object-like, non-variadic).

### Step 3: Tests

- Added `test/integration/test_pp_named_variadic_macro`: bare form with no fixed parameters, `#define REPEAT_TWICE(MEMBERS...) MEMBERS; MEMBERS`, mirroring __struct_group's shape. Verifies that the replacement text can use the named parameter.
- Added `test/integration/test_pp_named_variadic_macro_fixed_param`: mixed form with both fixed and variadic parameters, `#define LOG(fmt, args...) printf(fmt, args)`, mirroring the common GNU eprintf-style idiom.
- Both tests follow the stage-159 precedent of reproducing GCC/GNU extension syntax directly in self-contained macros rather than depending on real /usr/include headers.

### Step 4: Documentation

- Updated `docs/grammar.md` preprocessor notes section to document GNU named variadic macro parameter support: the two standard forms and the GNU named form, and that the replacement text uses the actual name (not `__VA_ARGS__`).
- Updated `docs/outlines/checklist.md` stage 173 section: flipped the sub-item "Named variadic macro parameters..." under the pre-existing unchecked TODO "GNU extension: `__VA_OPT__` and named variadic args" to indicate this sub-feature is now done; `__VA_OPT__` remains unimplemented.

### Step 5: Version, Self-Host, and Final Commit

- Incremented `src/version.c` to `01730000`.
- Ran full test suite: all 2074 portable tests pass (165 unit, 1286 valid, 261 invalid, 191 integration [189 + 2 new], 50 print-AST, 100 print-tokens, 21 print-asm); 191 system-include pass; 2 optional-library pass. No regressions.
- Committed code changes: `3a5211a — stage 173: GNU named variadic macro parameters (--sysinclude networking headers)`; `c6bcb29 — self-host C0 verified: all tests pass`; `6a34d64 — self-host C1 verified: all tests pass`; `790197c — docs: update self-compilation report for stage 173`.
- Executed `./build.sh --mode=self-host`: full C0→C1→C2 cycle completed cleanly. C0 (GCC-built, v00.03.01730000.01268) → C1 (v00.03.01730000.01269) → C2 (v00.03.01730000.01270). All three passed the full portable + system-include + optional-library suite with zero source changes needed during bootstrap (preprocessor.c changes use only malloc/free/strlen/strncmp/pointer assignment/ternary—plain C89/C99 constructs).
- Updated `docs/self-compilation-report.md` with stage-173 self-host results.

## Final Results

All 2074 portable tests pass (165 unit, 1286 valid, 261 invalid, 191 integration [189 existing + 2 new], 50 print-AST, 100 print-tokens, 21 print-asm). System-include suite: 191/191 pass. Optional-library suite: 2/2 pass. Self-host C0→C1→C2 verified with zero regressions. No source changes required during bootstrap.

## Session Flow

1. Read bug report and supporting details about the networking header failure.
2. Investigated the include search and reproduced the reported error with and without `--sysinclude`.
3. Performed header bisection to isolate the failing header in the include chain.
4. Identified the root cause: `/usr/include/linux/stddef.h` uses GNU named variadic macro syntax (`__struct_group(TAG, NAME, ATTRS, MEMBERS...)`).
5. Designed the fix: extend preprocessor to parse and store named variadic parameter names; update all substitution sites to use the actual name.
6. Implemented preprocessor changes in `src/preprocessor.c`: modified `MacroDef` struct, parser, `macro_define()`, and validator.
7. Updated all 30 predefined-macro call sites to pass NULL for the new parameter.
8. Created two new portable integration tests to verify both the bare and mixed (fixed + variadic) forms.
9. Ran full test suite: all tests pass.
10. Updated `docs/grammar.md` and `docs/outlines/checklist.md` to document the new feature.
11. Incremented version to `01730000` and committed code changes.
12. Ran `./build.sh --mode=self-host` for full C0→C1→C2 bootstrap verification; all tests pass at every stage.
13. Updated `docs/self-compilation-report.md` with stage-173 results.
14. Generated milestone summary, transcript, and README update.
