# Stage 60-02 Kickoff: Runtime/Context Predefined Macros

## Spec Summary

Add three predefined macros that provide runtime/context data, expanded by the preprocessor similarly to `__FILE__` and `__LINE__`:

- `__DATE__` — expands to a string literal in "Mmm DD YYYY" format (e.g., `"May 23 2026"`)
- `__TIME__` — expands to a string literal in "HH:MM:SS" format (e.g., `"14:30:00"`)
- `__STDC_HOSTED__` — expands to integer constant `1` (hosted implementation)

## Derived Stage Values

**Affected subsystems:**
- Preprocessor only (macro table initialization at compile time)

**Type of change:**
- Preprocessor enhancement (new predefined macros)

**Scope:**
- No tokenizer, parser, AST, codegen, or grammar changes required
- No public API changes

## Planned Changes

### Source Code Changes

**src/preprocessor.c:**
- Add `#include <time.h>` for time formatting
- Locate the predefined macros initialization section (where `__STDC__`, `__STDC_VERSION__`, `__CLAUDEC99__`, and stage 60-01 macros are injected)
- At startup of `preprocess_with_defines_and_includes()`:
  - Call `time(NULL)` and `localtime()` to get current date/time
  - Format `__DATE__` using `strftime()` with format `"%b %d %Y"` (e.g., "May 23 2026"), then quote it (e.g., `"\"May 23 2026\""`)
  - Format `__TIME__` using `strftime()` with format `"%H:%M:%S"` (e.g., "14:30:00"), then quote it (e.g., `"\"14:30:00\""`)
  - Inject all three as object-like macros via `macro_define()` calls
- `__STDC_HOSTED__` value is `1` (quoted as string, e.g., `"1"`)

### Tests

**test/valid/:**
- `test_predefined_macro_stdc_hosted__0.c` — verify `__STDC_HOSTED__ == 1`
- `test_predefined_macro_date__0.c` — verify `__DATE__` expands to a non-empty string (strlen > 0)
- `test_predefined_macro_time__0.c` — verify `__TIME__` expands to a non-empty string (strlen > 0)

## Implementation Order

1. Implement macro injection in src/preprocessor.c
2. Create valid test cases
3. Verify all tests pass

## Notable Decisions

- Macros are injected at compile-time with computed values (not statically hardcoded like stage 60-01)
- Date/time are captured once at preprocessor startup, not re-evaluated per expansion
- `__STDC_HOSTED__` is `1` because this is a hosted implementation (targets Linux with libc)
- String literals are quoted in the expansion value (matching standard C99 behavior)

## Known Ambiguities

None identified. Spec is clear on macro names, values, and implementation approach.

## Testing Strategy

- Test macro existence via direct value comparison
- Test that `__DATE__` and `__TIME__` produce non-empty strings
- Test that `__STDC_HOSTED__` evaluates to integer `1` in conditional expressions
