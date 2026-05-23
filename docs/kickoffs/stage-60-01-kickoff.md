# Stage 60-01 Kickoff: Static Predefined Target Macros

## Spec Summary

Add 12 static predefined target/ABI macros into the preprocessor macro table before preprocessing begins. These macros provide the compiler with platform identification and size information.

**Platform macros (5):**
- `__x86_64__` → 1
- `__linux__` → 1
- `__unix__` → 1
- `__LP64__` → 1
- `_LP64` → 1

**Size macros (7):**
- `__CHAR_BIT__` → 8
- `__SIZEOF_CHAR__` → 1
- `__SIZEOF_SHORT__` → 2
- `__SIZEOF_INT__` → 4
- `__SIZEOF_LONG__` → 8
- `__SIZEOF_POINTER__` → 8
- `__SIZEOF_SIZE_T__` → 8

## Derived Stage Values

**Affected subsystems:**
- Preprocessor only (macro table initialization)

**Type of change:**
- Preprocessor enhancement (new predefined macros)

**Scope:**
- No tokenizer, parser, AST, codegen, or grammar changes required
- No public API changes

## Planned Changes

### Source Code Changes

**src/preprocessor.c:**
- Locate the existing predefined macros section (where `__STDC__`, `__STDC_VERSION__`, and `__CLAUDEC99__` are injected)
- Add 12 new `macro_define` calls to inject the platform and size macros directly into the macro table
- Ensure macros are initialized before preprocessing begins

### Tests

**test/valid/:**
- 5 new valid tests to validate the new macros
- Tests should verify:
  - Macro presence via `#ifdef` checks
  - Macro values via `#if` checks
  - Basic usage in conditional compilation

## Implementation Order

1. Implement macro injection in src/preprocessor.c
2. Create valid test cases
3. Verify all tests pass

## Notable Decisions

- Macros are injected directly in source code (not from external header file)
- Values are fixed for the x86_64 Linux target platform
- Macros are added alongside existing predefined macros for consistency

## Known Ambiguities

None identified. Spec is clear on macro names, values, and implementation location.

## Testing Strategy

- Test `#ifdef` detection of each macro
- Test `#if` evaluation of macro values
- Test that macros are available in program source without explicit definition
