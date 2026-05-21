# stage-57-01 Transcript: Function-like Macro Stringification

## Summary

Stage 57-01 adds the stringification operator (`#`) to function-like macro replacement lists. When `#param` appears in a function-like macro, the actual argument tokens (not expanded) are converted to a C string literal. Whitespace is normalized to single spaces (with leading/trailing trim), and special characters (`"` and `\`) are escaped. The implementation adds validation at `#define` time to reject `#` in object-like macros, bare `#`, and `#` not followed by a parameter name.

## Changes Made

### Step 1: New `stringify_arg()` Function

- Converts raw argument text to a C string literal.
- Wraps the argument in double quotes.
- Normalizes whitespace: collapses runs of whitespace to a single space, trims leading and trailing whitespace.
- Escapes double quotes (`"` → `\"`).
- Escapes backslashes (`\` → `\\`).
- Returns allocated string; caller must free.

### Step 2: Extended `substitute_args()` Function

- Added two new parameters: `raw_args[]` (unexpanded argument text) and `raw_arg_lens[]` (lengths of raw arguments).
- Added string/char literal detection in replacement text scan: skips `"..."` and `'...'` to avoid incorrectly identifying `#` inside quoted strings.
- Implemented `#param` operator: when `#` is encountered followed by a parameter name, looks up the parameter index and calls `stringify_arg()` with the raw (pre-expansion) argument text.
- Returns the substituted text with stringified arguments in place.

### Step 3: `#define`-time Validation

- After parsing the replacement text, scans for `#` outside string/char literal content.
- Rejects `#` in object-like macros with error: `"error: hash in object like macro is not permitted"`.
- Rejects bare `#` (not followed by identifier) with error: `"error: bare hash in replacement list"`.
- Rejects `#` followed by non-parameter identifier with error: `"error: hash not followed by param 'name'"`.

### Step 4: Call Site Updates

- In `expand_macros_text()`: captures raw argument text in a separate array before entering the pre-expansion loop; passes raw args to `substitute_args()`; frees raw arg array after.
- In `preprocess_internal()`: same pattern for function-like macro calls within preprocessing: captures raw args, passes to `substitute_args()`, frees.

### Step 5: Tests

- Added 5 valid test cases:
  - `test_pp_str_simple__0.c`: `STR(coffee)` → `"coffee"`
  - `test_pp_str_expr__0.c`: `STR(1 + 2)` → `"1 + 2"`
  - `test_pp_str_whitespace_norm__0.c`: `STR(1     + 2)` → `"1 + 2"`
  - `test_pp_str_no_expand__0.c`: `STR(NAME)` → `"NAME"` (arg not macro-expanded)
  - `test_pp_str_escape__0.c`: `STR("HELLO")` → `"\"HELLO\""`
- Added 2 invalid test cases:
  - `test_invalid_140__hash_not_followed_by_param.c`: `#define BAD(x) #y` rejected
  - `test_invalid_141__bare_hash_in_replacement.c`: `#define BAD(x) #` rejected
  - `test_invalid_142__hash_in_object_like_macro.c`: `#define BAD #x` rejected

## Final Results

All 1022 tests pass. No regressions.

**Test breakdown:**
- Valid: 633 (628 prior + 5 new)
- Invalid: 199 (196 prior + 3 new)
- Integration: 31 (unchanged)
- Print-AST: 39 (unchanged)
- Print-tokens: 99 (unchanged)
- Print-asm: 21 (unchanged)
- **Total: 1022 (1014 prior + 8 new)**

Build succeeds without warnings. All compilation, assembly, and execution steps pass.

## Session Flow

1. Read stage spec and supporting documentation (spec, templates, README).
2. Reviewed preprocessor code in `src/preprocessor.c` to understand macro expansion architecture.
3. Planned implementation: stringification in three phases (stringify function, substitution, validation).
4. Implemented `stringify_arg()` to convert raw argument tokens to C string literal with whitespace normalization and character escaping.
5. Extended `substitute_args()` to accept raw argument arrays and handle `#param` operator with literal detection.
6. Added `#define`-time validation to reject invalid `#` usage in object-like and function-like macros.
7. Updated call sites in `expand_macros_text()` and `preprocess_internal()` to capture and pass raw arguments.
8. Created and verified 7 new test cases (5 valid, 3 invalid) covering specification requirements.
9. Ran full test suite: 1022 passed, 0 failed.
10. Generated milestone summary and transcript artifacts.

## Notes

- **Argument non-expansion**: Arguments are stringified before macro expansion, so `STR(NAME)` where `NAME` is a macro expands to `"NAME"` (the text), not the macro's replacement. This matches C99 standard behavior.
- **Whitespace normalization**: Runs of whitespace (spaces, tabs, newlines) are collapsed to a single space; leading and trailing whitespace is trimmed. This ensures consistent string output regardless of whitespace in the invocation.
- **Out-of-scope features**: Token pasting (`##`), variadic macros, and advanced rescan behavior remain unimplemented as per stage scope.

