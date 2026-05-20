# stage-56-04 Transcript: Angle-Bracket Includes

## Summary

This stage adds support for `#include <filename>` (angle-bracket include syntax) to the preprocessor. Unlike quoted includes (`#include "filename"`), which search the directory of the including file first and then `-I` directories, angle-bracket includes search only in the `-I` directories provided on the command line, in the order they are specified. This aligns with standard C preprocessor behavior and allows cleaner separation between local and system-like includes.

The implementation adds a new `resolve_angle_include_path()` function to the preprocessor that handles the angle-bracket search strategy, while the existing `resolve_include_path()` function continues to handle quoted includes with its directory-of-including-file-first semantics.

## Changes Made

### Step 1: Preprocessor Directive Parsing

- Extended the `#include` directive handler in `src/preprocessor.c` to recognize and distinguish between `<filename>` and `"filename"` forms
- The lexer now captures both angle-bracket and quoted include syntax
- The parser branches to different resolution functions based on the include form detected

### Step 2: Path Resolution for Angle-Bracket Includes

- Implemented `resolve_angle_include_path()` function in `src/preprocessor.c`
- This function searches only the `-I` directories (from `include_dirs` list) in command-line order
- Does not search the directory of the including file, unlike quoted includes
- Returns the resolved file path or NULL if not found in any `-I` directory

### Step 3: Error Handling and Messaging

- Updated error messages to distinguish between the two include forms
- Angle-bracket not-found errors display as `<name>` (without quotes)
- Quoted not-found errors display as `"name"` (with quotes)
- Both error cases preserve the original filename in diagnostics

### Step 4: Documentation Updates

- Updated the docstring of `preprocess_with_defines_and_includes()` in `include/preprocessor.h` to document both quoted and angle-bracket search orders
- Grammar already contained both include forms in the EBNF rule for `<include_directive>`
- README.md updated to describe the preprocessor's support for angle-bracket includes

### Step 5: Integration Testing

- Added new integration test directory `test/integration/test_pp_angle_include/`
- Test structure:
  - `include/add.h`: declares `int add(int a, int b);`
  - `add.c`: implements `add` using `#include <add.h>`
  - `test_pp_angle_include.c`: calls `add(3, 4)` using `#include <add.h>`
  - `test_pp_angle_include.cflags`: specifies `-Iinclude` to make `add.h` discoverable
  - `test_pp_angle_include.status`: expects exit code 7 (3 + 4)
- Test verifies both that angle-bracket includes work and that the `-I` search path is correctly applied

## Final Results

All tests pass. The integration test suite increased from 30 to 31 tests. Full suite aggregate: 1010/1010 pass (625 valid, 195 invalid, 31 integration, 39 print-AST, 99 print-tokens, 21 print-asm). No regressions.

## Session Flow

1. Read spec and supporting files to understand angle-bracket include requirements and search semantics
2. Reviewed existing `#include` handling in preprocessor to understand quoted-include path resolution
3. Identified the distinction: quoted includes search source-dir-first, angle-bracket includes search `-I`-only
4. Implemented `resolve_angle_include_path()` to search `-I` directories without the source-directory fallback
5. Extended the include directive handler to dispatch to the appropriate resolution function based on syntax form
6. Updated error messages to display the correct delimiters for each form
7. Added the integration test case to validate both the parsing of angle-bracket syntax and the `-I` search behavior
8. Updated documentation (README.md and preprocessor.h docstring) to describe the behavior
9. Verified that grammar.md already documented both include forms
10. Built and ran full test suite to confirm all 1010 tests pass with no regressions
