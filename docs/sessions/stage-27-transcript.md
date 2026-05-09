# stage-27 Transcript: Declaration Specifier Refactor

## Summary

Stage 27 refactors the parser's declaration specifier handling to accept a list of declaration specifiers instead of a fixed structure of optional storage class followed by a single type specifier. This change enables stricter validation of duplicate specifiers and provides a foundation for future language extensions. The refactor introduces a new `DeclSpecResult` struct and a `parse_declaration_specifiers()` helper function that iterates over all declaration specifier tokens, tracking both storage class and type with duplicate detection. The external declaration parser now calls this helper instead of performing inline storage class and type parsing.

## Changes Made

### Step 1: Grammar Update

- Updated `<declaration_specifiers>` production rule from `[ <storage_class_specifier> ] <type_specifier>` to `<declaration_specifier> { <declaration_specifier> }`
- Added new `<declaration_specifier>` production that is either a `<storage_class_specifier>` or a `<type_specifier>`
- Updated documentation in `docs/grammar.md` to reflect the new rules

### Step 2: Parser Refactor

- Added `DeclSpecResult` struct to track parsed storage class, type specifier, and computed type
- Implemented `parse_declaration_specifiers()` helper function that:
  - Loops over declaration specifier tokens (extern, static, char, short, int, long)
  - Tracks the first storage class seen and rejects additional ones with error "multiple storage class specifiers are not allowed"
  - Tracks the first type specifier seen and rejects additional ones with error "multiple type specifiers are not allowed"
  - Returns error "expected type specifier" if no type is provided
  - Returns a `DeclSpecResult` with the parsed specifiers and computed type
- Updated `parse_external_declaration()` to call the new helper instead of inline parsing
- Updated function comment to document the new grammar

### Step 3: Test Coverage

- Added test_invalid_135: `static extern int x;` — detects multiple storage class specifiers
- Added test_invalid_136: `extern static int x;` — detects multiple storage class specifiers (reverse order)
- Added test_invalid_137: `int int x;` — detects multiple type specifiers
- Added test_invalid_138: `static char int x;` — detects multiple type specifiers after storage class

## Final Results

Build successful. All 573 tests pass:
- Valid tests: 436 passed (432 existing + 4 tests that use new parsing)
- Invalid tests: 137 passed (133 existing + 4 new)

No regressions. All existing valid programs continue to compile correctly.

## Session Flow

1. Read stage spec and reviewed current parser implementation
2. Studied grammar rules and existing declaration specifier parsing
3. Designed `DeclSpecResult` struct to encapsulate parsed specifiers
4. Implemented `parse_declaration_specifiers()` helper with loop-based list parsing
5. Integrated helper into `parse_external_declaration()`
6. Updated `docs/grammar.md` with new production rules
7. Added four new invalid test cases for duplicate detection
8. Built compiler and ran full test suite
9. Verified all 573 tests pass with no regressions
10. Generated milestone, transcript, and README artifacts
