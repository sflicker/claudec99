# stage-159 Transcript: SDL2 Compile Failure: GCC Extension Parsing Fixes

## Summary

Stage 159 fixes a critical compile error when processing SDL2 headers: `/usr/include/SDL2/SDL_main.h:152:50: error: expected ')', got '[' ('[')`. The root cause is a function-pointer typedef with array parameters: `typedef int (*SDL_main_func)(int argc, char *argv[]);`. The parser's three inline parameter-list parsing loops in `parse_declarator` consumed optional parameter names but did not handle `[]` array-suffix forms after those names. Per C99 §6.7.5.3p7, array parameters automatically adjust to pointers, so the parser must recognize and consume `[...]` suffixes and increment the pointer level. Additionally, SDL2 headers use GCC extensions that required five complementary fixes: anonymous struct/union members without declarators, `__attribute__` and `__declspec` no-op macros, `__extension__` predefined macro, and `__asm__` / `asm` statement parsing. A bootstrap issue was also discovered during self-hosting: the preprocessor preamble contained adjacent string literals that C0 does not support, resolved by merging them into a single string.

## Changes Made

### Step 1: Array Parameters in Function-Pointer Typedef Parameter Lists

- **Core fix**: Three inline parameter-type parsing loops in `src/parser.c` `parse_declarator` function (~lines 1460, 1500, 1583) now each contain a `while (TOKEN_LBRACKET)` loop that consumes `[...]` suffixes after parameter names.
- **Loop behavior**: For each `[` consumed, skip any dimension expression (if present; may be empty), consume the matching `]`, and increment the pointer level (`inner_stars++`) to implement C99 array-to-pointer decay.
- **Affected declarator forms**:
  1. Main function-pointer parameter loop (around line 1583): processes `int argc, char *argv[]` in `typedef int (*SDL_main_func)(...)`
  2. "Own params" loop for functions-returning-fp (around line 1460): handles return-type function signature
  3. "Returned fp params" loop for functions-returning-fp (around line 1500): handles parameter-list signature
- **Result**: `char *argv[]` is now correctly parsed as a pointer to pointer to char (three pointer levels: array→ptr, ptr, ptr-to-char).

### Step 2: Trailing Type Qualifiers in Parameter Declarations

- **Change**: After `parse_type_specifier` in both `parse_parameter_declaration` and all three inline parameter-type parsing loops, consume and discard any trailing `const`, `volatile`, or `restrict` tokens.
- **Reason**: SDL2 and other system headers use patterns like `void const *ptr` and `int volatile x`, which are valid C99 but the parser previously rejected them.
- **Result**: Qualifiers after the base type no longer cause parse errors.

### Step 3: Anonymous Struct/Union Members Without Declarators

- **Problem**: `struct { union { ... }; }` with no declarator name after the inner union/struct now compiles without error.
- **Fix**: In both `parse_struct_specifier` and `parse_union_specifier`, after parsing a nested struct/union body, check if the next token is `;` (instead of an identifier). If so, skip past the `;` without creating a field entry.
- **Result**: Anonymous nested struct/union members are silently accepted (common in system headers for padding and variant field grouping).

### Step 4: `__asm__` and `asm` Statement Parsing and Skipping

- **Scope**: Added to `parse_statement` in the top-level switch to handle inline assembly as a statement form.
- **Parsing**: When `__asm__` or `asm` keyword is encountered, consume it, check for optional `volatile` keyword, then consume a balanced parenthesized argument block (the assembly code), and return an empty `AST_BLOCK` node.
- **Result**: Programs using inline assembly (common in system headers for platform-specific operations) no longer fail at parse time; the assembly is discarded and replaced with a no-op.

### Step 5: `__extension__` Predefined Macro

- **Location**: `src/preprocessor.c` predefined macro table.
- **Definition**: `#define __extension__` with empty expansion.
- **Effect**: GCC `__extension__ union { ... };` and similar constructs expand to just the following tokens, allowing the construct to parse without the extension keyword.

### Step 6: `__attribute__` and `__declspec` No-op Macros

- **Location**: `src/preprocessor.c` `builtin_preamble` (injected before all source files).
- **Definitions**: Two single-token macro definitions:
  - `#define __attribute__(x)` (no replacement body; `x` is consumed and discarded)
  - `#define __declspec(x)` (no replacement body; `x` is consumed and discarded)
- **Bootstrap fix**: C0 does not support adjacent string literal concatenation, so the preamble string was split across multiple lines; the fix merged all parts into one contiguous `const char *` string literal.
- **Result**: Function declarations with `__attribute__((noinline))` and similar GCC/MSVC extension attributes parse without error.

### Step 7: Version Update

- **File**: `src/version.c`
- **Change**: VERSION_STAGE incremented from `"01580000"` to `"01590000"`

### Step 8: Integration Tests

Five new test directories added under `test/integration/`:

1. **test_fp_array_param**: Function pointer typedef with array parameter; verifies `typedef int (*func_t)(int argc, char *argv[])` compiles and can be called.
2. **test_gcc_attribute_noop**: Function declaration with `__attribute__((noinline))`; verifies the attribute is silently consumed without affecting code generation.
3. **test_gcc_asm_skip**: Inline `__asm__ volatile (...)` and `asm (...)` statements; verifies they parse and are replaced with no-ops.
4. **test_gcc_anon_union_skip**: Anonymous union member inside a struct body; verifies the nested union is accepted without a declarator name.
5. **test_trailing_qualifier**: Trailing `const`/`volatile` qualifiers in function parameters and variable declarations; verifies `void const *` parameters and similar forms parse correctly.

All tests compile and execute successfully.

## Final Results

**Build Status**: No errors; all changes compile cleanly with GCC and Clang at `-std=c99 -pedantic-errors`.

**Test Results**:
- Portable (all platforms): 2061 passed, 0 failed (was 2056; added 5 integration tests)
  - Breakdown: 165 unit, 1286 valid, 261 invalid, 178 integration, 50 print-AST, 100 print-tokens, 21 print-asm
- System-include (Linux x86_64): 178 passed, 0 failed
  - Includes smoke tests for SDL2 header parsing and other system headers

**Regression Status**: No regressions; all previously passing tests continue to pass.

**Self-hosting**: C0→C1→C2 cycle completed successfully.
- **Bootstrap issue fixed**: Adjacent string literals in `builtin_preamble` merged into one (C0 does not support implicit string concatenation).
- **C0 version**: `00.03.01590000.01181` (2239/2239 tests)
- **C1 version**: `00.03.01590000.01182` (2239/2239 tests)
- **C2 version**: `00.03.01590000.01183` (2239/2239 tests)

## Session Flow

1. Read stage spec and reviewed SDL2 header failure root cause analysis
2. Analyzed current `parse_declarator` implementation and identified three parameter-list parsing loops
3. Implemented array-suffix handling (`[...]` loop) in all three parameter-list locations
4. Added trailing type-qualifier consumption after `parse_type_specifier` calls
5. Implemented anonymous struct/union member skip in both `parse_struct_specifier` and `parse_union_specifier`
6. Implemented `__asm__` / `asm` statement parsing in `parse_statement`
7. Added `__extension__`, `__attribute__`, and `__declspec` predefined macros to preprocessor
8. Created five integration tests covering all six fixes
9. Verified all 2061 portable tests pass and no regressions
10. Ran self-host build, discovered and fixed bootstrap issue with adjacent string literals in preprocessor preamble
11. Verified C0→C1→C2 cycle completes with all tests passing at each stage

## Notes

**SDL2 Limitations**: Although Stage 159 fixes the primary parse error, SDL2 compilation to completion requires disabling intrinsic headers via `SDL_DISABLE_IMMINTRIN_H`. The intrinsic headers (xmmintrin.h, bmi2intrin.h, sgxintrin.h) require:
- `unsigned __int128` (128-bit GCC extension, not in C99)
- Vector types like `__m128` with compound literal initializers
- `enum` types used as asm constraint specifiers

These are fundamentally out of scope for a C99-compliant compiler. With the environment variable `SDL_DISABLE_IMMINTRIN_H=1` set during compilation, SDL2 headers parse successfully and the compiler can proceed to later stages (code generation, linking).

**Parser Simplicity**: The fixes maintain the incremental, hand-coded parser design without introducing new AST node types or complex precedence machinery. All changes are localized to three parsing functions and the preprocessor macro table.
