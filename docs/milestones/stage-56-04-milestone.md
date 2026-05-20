# Milestone Summary

## Stage 56-04: Angle-Bracket Includes - Complete

stage-56-04 ships support for `#include <filename>` (angle-bracket include syntax) with search paths limited to `-I` directories in command-line order.

- **Preprocessor**: Extended the `#include` directive handler to accept both `"filename"` (quoted) and `<filename>` (angle-bracket) syntax. Quoted includes search the directory of the including file first, then `-I` directories in command-line order. Angle-bracket includes search `-I` directories only, in command-line order. Added `resolve_angle_include_path()` function to implement angle-bracket path resolution. Error messages distinguish between the two forms with appropriate delimiters.
- **Grammar/Parser**: No changes; both include forms were already documented in the EBNF grammar.
- **AST/Semantics**: No changes; preprocessing is pre-parse.
- **Codegen**: No changes.
- **Tests**: One new integration test added (`test_pp_angle_include`). Full suite: 1010/1010 pass (625 valid, 195 invalid, 31 integration, 39 print-AST, 99 print-tokens, 21 print-asm).
- **Docs**: README.md updated to reflect angle-bracket include behavior; grammar.md already had both include forms documented.
