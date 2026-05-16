# stage-50 Transcript: Object-like Macros

## Summary

Stage 50 adds object-like `#define` macro support to the preprocessor. The implementation introduces a shared `MacroTable` (dynamic array of macro name/replacement pairs) that is threaded through all recursive preprocessing calls, enabling macros defined in included headers to be visible to the including file. Macro names (identifiers) in ordinary source text outside string/character literals are expanded by simple text replacement with no re-scanning. Compatible macro redefinitions are silently allowed; incompatible ones produce a fatal error with diagnostic message. Identifier-level expansion outputs replacement text verbatim, preserving all tokens and operators as-is.

## Changes Made

### Step 1: Preprocessor Macro Infrastructure

- Added `MacroTable` struct (dynamic array of `MacroDef` entries) to `include/preprocessor.h` and `src/preprocessor.c`.
- Added `MacroDef` struct containing `name` and `replacement_text` fields for storing macro definitions.
- Implemented `macro_table_init()` to allocate and initialize an empty macro table.
- Implemented `macro_table_free()` to deallocate macro table and all stored strings.
- Implemented `macro_find(table, name)` to search for a macro by name.
- Implemented `macro_define(table, name, replacement)` to add or update a macro, with validation for compatible redefinition.

### Step 2: #define Directive Parsing

- Updated preprocessor directive parser to recognize and handle `#define` syntax: `#define NAME replacement-list`.
- Extracted macro name and replacement text from `#define` directives.
- Called `macro_define()` to store the macro in the shared `MacroTable`.
- Fatal error "incompatible replacement" emitted when attempting to redefine a macro with different replacement text.
- Removed special "unsupported preprocessor directive" error for `#define` (now supported).

### Step 3: Macro Expansion in Ordinary Source

- Implemented identifier-level macro expansion in the main preprocessing loop.
- When an identifier token is encountered in ordinary source text (outside string and character literals), checked `MacroTable` for a matching macro.
- If found, replaced the identifier with the macro's replacement text (output verbatim, no re-scanning or token re-parsing).
- Ensured expansion does not occur inside string literals (e.g., `"ANSWER"`) or character literals (e.g., `'A'`).
- Preserved all operator and punctuation tokens in replacement text unmodified.

### Step 4: Cross-File Macro Visibility

- Modified preprocessing recursion for included files to pass the same `MacroTable` through all recursive calls.
- Ensured macros defined in `#include "header.h"` are visible to the including `.c` file.
- Macros accumulate in the shared table as includes are processed.

### Step 5: Test Suite

- Added `test/valid/test_pp_define_value__42.c`: Single macro definition and use.
- Added `test/valid/test_pp_define_array__42.c`: Macro used in array declaration.
- Added `test/valid/test_pp_define_sum_expr__10.c`: Macro with expression replacement (`1+2+3+4`).
- Added `test/valid/test_pp_define_compat_redef__42.c`: Compatible redefinition of same macro.
- Added `test/valid/test_pp_define_stmt_macros__2.c`: Multiple macros used in statement contexts.
- Added `test/integration/test_pp_define_header/`: Multi-file test with macro defined in `constants.h` and used in main file.
- Added `test/invalid/test_pp_define_incompat_redef__incompatible_replacement.c`: Redefining macro with different replacement text (fatal error expected).
- Deleted `test/invalid/test_pp_unsupported_define__unsupported_preprocessor_directive.c`: Now supported.
- Deleted `test/invalid/test_pp_unsupported_include__unsupported_preprocessor_directive.c`: Now supported.

### Step 6: Documentation

- Updated `include/preprocessor.h` doc comment to describe `#define` macro support, macro visibility across includes, compatible redefinition policy, and expansion semantics.
- Added preprocessing section to `docs/grammar.md` documenting object-like `#define` syntax, macro storage and expansion, empty macros, redefinition rules, and scope.

## Final Results

Build: successful, no compilation errors.

Test results: All 907 tests pass:
- valid: 542 passed, 0 failed, 542 total
- invalid: 181 passed, 0 failed, 181 total
- integration: 25 passed, 0 failed, 25 total
- print_ast: 39 passed, 0 failed, 39 total
- print_tokens: 99 passed, 0 failed, 99 total
- print_asm: 21 passed, 0 failed, 21 print_asm

New tests all pass (all exit codes correct or error messages matched).
No regressions: all existing tests continue to pass.

## Session Flow

1. Read stage spec and supporting documentation (templates, grammar reference).
2. Reviewed existing preprocessor implementation to understand macro table integration points.
3. Examined test cases to confirm expected behavior for single-file and multi-file macro usage.
4. Designed `MacroTable` data structure and macro lookup/define functions.
5. Implemented `#define` directive parsing and storage in preprocessor.
6. Implemented identifier-level macro expansion in ordinary source text.
7. Threaded `MacroTable` through recursive preprocessing calls for included files.
8. Built compiler and ran full test suite.
9. Verified all 907 tests pass and no regressions occurred.
10. Generated post-implementation artifacts (milestone, transcript, grammar and README updates).
