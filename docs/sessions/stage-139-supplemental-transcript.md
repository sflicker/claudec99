# stage-139-supplemental Transcript: System Header Compatibility (sysinclude 52→98/98)

## Summary

Stage 139 Supplemental fixed eight critical system header compatibility issues that prevented the compiler from parsing glibc headers. The `run_tests_sysinclude.sh` test score improved from 52/98 to 98/98 (perfect score) across eight focused bug fixes: removal of conflicting GCC extension predefinitions, block comment stripping from macro replacement text, function-type typedef support, `long double` type specifier parsing, unnamed array parameters in function prototypes, cast expressions in constant expressions, injection control for the preamble in `--print-tokens` mode, and self-host fix for adjacent string literal concatenation. The core challenge was matching system header idioms without relying on planned-future features like K&R-style function declarations.

## Changes Made

### Step 1: Conflicting GCC Extension Predefinitions Removed

- Identified that glibc's `sys/cdefs.h` defines `__inline`, `__attribute__`, and `__extension__` as macros, which conflicted with ClaudeC99's builtin preamble predefinitions.
- Removed `__inline`, `__attribute__`, and `__extension__` from the builtin preamble in `src/preprocessor.c`.
- Retained `__restrict__`, `__volatile__`, and `__const__` because glibc's `cdefs.h` does not define them.
- Fixes `error: macro '__inline' redefined with incompatible replacement text` when parsing headers.

### Step 2: Block Comment Stripping from Macro Replacement Text

- Added `strip_block_comments()` helper function in `src/preprocessor.c` to remove `/* ... */` comment sequences from macro replacement text before storage in the MacroTable.
- glibc headers like `math.h` use patterns such as `#define __attribute_malloc__ /* Ignore */`, where the comment was being stored as raw tokens and later appearing in compiler output.
- Applies stripping to both object-like and function-like macros during the macro definition processing.
- Fixes `expected ';', got '/'` errors caused by comments appearing as tokens in compiled output.

### Step 3: Function-Type Typedef Support

- Extended `parse_type_specifier` in `src/parser.c` to recognize and parse function-type typedefs: `typedef ret name(params);`
- System headers like `cookie_io_functions_t.h` contain patterns such as `typedef ssize_t cookie_read_function_t(void *, char *, size_t);`
- Parser now detects function prototype parentheses following a typename and parses the parameter list via a paren-balanced skip loop.
- Registers the name as a TYPE_LONG placeholder in the typedef table so that uses like `name *fp` in struct bodies compile correctly without a full function-to-pointer type representation.
- Applied at both block-scope and file-scope typedef sites.
- Fixes `error: only scalar, pointer, and array typedefs are supported` errors.

### Step 4: `long double` Type Specifier Support

- Added check for `TOKEN_DOUBLE` in the `TOKEN_LONG` case of `parse_type_specifier` in `src/parser.c`.
- Recognizes `long double` as a valid type specifier (present in `bits/floatn-common.h` patterns like `typedef long double _Float64x;`).
- Treated as `TYPE_DOUBLE` for code generation purposes (ClaudeC99's floating-point codegen is uniform across all FP types).
- Fixes `error: expected type specifier` when parsing headers containing `long double` declarations.

### Step 5: Unnamed Array Parameters in Function Prototypes

- Modified `parse_parameter_declaration` in `src/parser.c` to handle unnamed array parameters without preceding identifiers (e.g., `extern char *tmpnam(char[L_tmpnam]);`).
- Added early-return case that detects `TOKEN_LBRACKET` at the start of a parameter and skips dimensions via a bracket-balanced loop.
- Adjusts the parameter type from array to pointer before returning.
- Fixes constraint violations when system headers use unnamed array dimensions in extern function prototypes.

### Step 6: Cast Expressions in Constant Expressions

- Modified `eval_const_primary` in `src/preprocessor.c` to distinguish between grouped expressions `(expr)` and casts `(type)expr`.
- Detects `(type-keyword)` patterns by checking if the next token after the opening paren is a type keyword (e.g., `int`, `size_t`).
- If a type keyword is detected, calls `parse_type_name` to parse the type, consumes the closing paren via `parser_expect(RPAREN)`, then calls `eval_const_unary` to evaluate the cast operand.
- Added forward declaration of `eval_const_unary` to resolve function ordering dependency.
- System headers contain patterns like `__NFDBITS = 8 * (int) sizeof(__fd_mask)`.
- Fixes `error: unexpected token ')'` when cast expressions appear in `#define` constant expressions.

### Step 7: Injection Control for Preamble in Print-Tokens Mode

- Added `inject_preamble` parameter to `preprocess_with_defines_and_includes` in `include/preprocessor.h` and `src/preprocessor.c`.
- Modified `src/compiler.c` to pass `inject_preamble = !print_tokens`: when `--print-tokens` mode is active, the preamble is not injected.
- The `__builtin_va_list` definition from the preamble was being injected into diagnostic output, breaking ~150 print-tokens expected files.
- Fixes output mismatch in all `--print-tokens` test suites.

### Step 8: Self-Host Fix for Adjacent String Literal Concatenation

- Fixed `src/preprocessor.c` builtin_preamble string initialization: adjacent string literal concatenation was split across multiple lines and relied on C99 string literal concatenation feature (not yet implemented in ClaudeC99).
- Collapsed the preamble into a single long string to allow self-compilation.
- Committed test `test/integration/test_long_unsigned_ordering/` that was generated during the earlier stage-139 session.
- Updated `docs/self-compilation-report.md` with C0=01046, C1=01047, C2=01048 version markers.

## Final Results

System header compatibility (run_tests_sysinclude.sh): **98/98** (perfect score), improved from 52/98 at session start. Regular test suite: all 1979 tests pass (1284 valid, 262 invalid, 97 integration, 50 print-AST, 100 print-tokens, 21 print-asm; 165 unit). No regressions. Self-host cycle C0→C1→C2 verified; all 1979 tests pass at every stage with no source modifications required during bootstrap.

## Session Flow

1. Read stage context and system header compatibility requirements.
2. Analyzed `run_tests_sysinclude.sh` failures to identify root causes (52/98 passing).
3. Implemented Step 1: removed conflicting GCC predefinitions from preamble.
4. Implemented Step 2: added `strip_block_comments()` helper to remove comments from macro text.
5. Implemented Step 3: extended `parse_type_specifier` to handle function-type typedefs at both scopes.
6. Implemented Step 4: added `long double` type specifier recognition in parser.
7. Implemented Step 5: modified `parse_parameter_declaration` to skip unnamed array dimensions.
8. Implemented Step 6: modified `eval_const_primary` to parse cast expressions in constant expressions.
9. Implemented Step 7: added `inject_preamble` parameter to control preamble injection in `--print-tokens` mode.
10. Implemented Step 8: fixed builtin preamble string concatenation for self-host compatibility.
11. Ran full test suite and `run_tests_sysinclude.sh` — achieved 98/98 system header compatibility score.
12. Performed self-host C0→C1→C2 cycle verification — all tests pass with no source changes.
