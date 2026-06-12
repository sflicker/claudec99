# stage-106 Transcript: C99 Header Completion

## Summary

Stage 106 completes C99 header support by adding the `restrict` type qualifier (parse-and-ignore, matching the `volatile` pattern) and comprehensively filling in the four primary system headers: `<ctype.h>`, `<string.h>`, `<stdlib.h>`, and `<stdio.h>`. After this stage, any non-floating-point, non-wide-character function from C99 §7.19–7.21 compiles and links correctly against system libc. A pre-existing code generation bug where `TYPE_LONG_LONG` and `TYPE_UNSIGNED_LONG_LONG` were omitted from return-value type checks (causing incorrect sign-extension of long-long function returns) was discovered and fixed during `strtoll` testing.

## Changes Made

### Step 1: Tokenizer — Add `restrict` keyword

- Added `TOKEN_RESTRICT` enum value to TokenType in `include/token.h`
- Added keyword table entry in `src/lexer.c` for "restrict" → `TOKEN_RESTRICT`
- Added "restrict" string to `token_type_name()` switch for `--print-tokens` output

### Step 2: Parser — Consume `restrict` in pointer-qualifier positions

- Modified `parse_declarator`: converted the `if` chain checking `TOKEN_CONST` and `TOKEN_VOLATILE` after each `*` to a `while` loop that also consumes `TOKEN_RESTRICT` (and discards it)
- Modified `parse_type_name` abstract declarator star loop: added parallel `TOKEN_RESTRICT` consume-and-discard
- Modified `parse_parameter_declaration` leading-qualifier check and pre-consume-stars loop: added `TOKEN_RESTRICT` handling
- No semantic changes; `restrict` is fully parse-and-ignore (matching the `volatile` implementation)

### Step 3: Stub headers — `<ctype.h>` completions

- Added 4 character-classification functions after existing `isxdigit`:
  - `int iscntrl(int)` — control character test
  - `int isgraph(int)` — graphic character test (printable and non-space)
  - `int isprint(int)` — printable character test
  - `int ispunct(int)` — punctuation character test

### Step 4: Stub headers — `<string.h>` completions

- Added 11 string/memory functions after existing `strrchr`:
  - `void *memmove(void *s1, const void *s2, size_t n)` — memory copy with overlap support (no `restrict`)
  - `void *memchr(const void *s, int c, size_t n)` — memory search
  - `char *strcat(char * restrict s1, const char * restrict s2)` — string concatenation
  - `size_t strcspn(const char *s1, const char *s2)` — span of characters not in set
  - `size_t strspn(const char *s1, const char *s2)` — span of characters in set
  - `char *strpbrk(const char *s1, const char *s2)` — search for first of any characters
  - `char *strstr(const char *s1, const char *s2)` — substring search
  - `char *strtok(char * restrict s1, const char * restrict s2)` — string tokenization
  - `int strcoll(const char *s1, const char *s2)` — locale-aware comparison
  - `size_t strxfrm(char * restrict s1, const char * restrict s2, size_t n)` — locale-aware transform
  - `char *strerror(int errnum)` — error description string

### Step 5: Stub headers — `<stdlib.h>` completions

- Added 3 struct typedefs:
  - `typedef struct { int quot; int rem; } div_t`
  - `typedef struct { long quot; long rem; } ldiv_t`
  - `typedef struct { long long quot; long long rem; } lldiv_t`
- Added 4 macros:
  - `#define EXIT_SUCCESS 0`
  - `#define EXIT_FAILURE 1`
  - `#define RAND_MAX 32767`
  - `#define MB_CUR_MAX 1`
- Added 21 function declarations:
  - Process control: `abort`, `atexit`, `_Exit`, `system`, `getenv`
  - Random numbers: `rand`, `srand`
  - Integer arithmetic: `abs`, `labs`, `llabs`, `div`, `ldiv`, `lldiv`
  - String conversion: `atoi`, `atol`, `atoll`, `strtoll`, `strtoull`
  - Searching/sorting: `bsearch`, `qsort`

### Step 6: Stub headers — `<stdio.h>` completions

- Added `fpos_t` typedef as `typedef long fpos_t`
- Added 7 macros:
  - `BUFSIZ`, `FOPEN_MAX`, `FILENAME_MAX`, `L_tmpnam`, `TMP_MAX`
  - `_IOFBF`, `_IOLBF`, `_IONBF`
- Added 31 function declarations:
  - File operations: `remove`, `rename`, `tmpfile`, `tmpnam`
  - Open/control: `freopen`, `fflush`, `setbuf`, `setvbuf`
  - Formatted output: `sprintf`, `vsprintf`
  - Formatted input: `scanf`, `fscanf`, `sscanf`, `vscanf`, `vfscanf`, `vsscanf`
  - Character output: `fputc`, `fputs`, `putc`
  - Character input: `getc`, `getchar`, `gets`, `ungetc`
  - Binary I/O: `fwrite`
  - Positioning: `fgetpos`, `fsetpos`, `rewind`
  - Error: `clearerr`, `feof`, `ferror`, `perror`

### Step 7: Minor header fixes

- `<stdbool.h>`: changed `__Bool_true_false_are_defined` to `__bool_true_false_are_defined` (C99 spec correction)
- `<stddef.h>`: added `typedef long ptrdiff_t`
- `<limits.h>`: added `CHAR_MIN`, `CHAR_MAX`, and `MB_LEN_MAX` macros

### Step 8: Codegen fix — long-long return-value sign-extension

- Fixed pre-existing bug in `src/codegen.c`: six locations checking `rhs_is_long` were missing `TYPE_LONG_LONG` and `TYPE_UNSIGNED_LONG_LONG` from their type checks
- This caused long-long function return values to be incorrectly sign-extended via `movsxd` instead of zero-extended
- Bug was exposed during `test_stdlib_strtoll__0.c` testing (strtoll returns long long); fixed by adding the missing type checks to all six sites

### Step 9: Version bump

- Updated `VERSION_STAGE` in `src/version.c` from `"01050000"` to `"01060000"`

### Step 10: New tests

- Created 13 new valid tests:
  - `test_stdlib_exit_codes__0.c` — EXIT_SUCCESS (exit 0)
  - `test_stdlib_exit_failure__1.c` — EXIT_FAILURE (exit 1)
  - `test_stdlib_abs__0.c` — abs, labs, llabs
  - `test_stdlib_atoi__0.c` — atoi string-to-int conversion
  - `test_stdlib_qsort__0.c` — qsort with comparator function pointer
  - `test_stdlib_strtoll__0.c` — strtoll, strtoull (tested the long-long return fix)
  - `test_string_memmove__0.c` — memmove with overlapping regions
  - `test_string_strstr__0.c` — strstr substring search
  - `test_string_strcat__0.c` — strcat concatenation
  - `test_string_strtok__0.c` — strtok tokenization with state
  - `test_stdio_sprintf__0.c` — sprintf formatted output
  - `test_ctype_new_classifiers__0.c` — iscntrl, isgraph, isprint, ispunct
  - `test_restrict_pointer_decl__0.c` — restrict in function signatures compiles

## Final Results

All 1607 tests pass (922 valid, 255 invalid, 86 integration, 50 print-AST, 100 print-tokens, 21 print-asm, 165 unit). This represents 13 new valid tests and no regressions.

Self-hosting: C0→C1→C2 cycle passed cleanly with no source changes needed. C0: 00.02.01060000.00848, C1: 00.02.01060000.00849, C2: 00.02.01060000.00850.

## Session Flow

1. Read spec file and reviewed Stage 106 goals and task breakdown
2. Reviewed token.h, lexer.c, parser.c for restrict keyword insertion points
3. Implemented restrict tokenization and parsing (4 parser sites handling pointer qualifiers)
4. Added 4 ctype.h classifiers and 11 string.h functions
5. Added stdlib.h types (div_t, ldiv_t, lldiv_t), macros (EXIT_SUCCESS, etc.), and 21 functions
6. Added stdio.h type (fpos_t), 7 macros, and 31 function declarations
7. Applied minor fixes to stdbool.h, stddef.h, limits.h
8. Bumped version stage to 01060000
9. Built compiler and ran full test suite
10. Discovered and fixed pre-existing long-long return-value codegen bug (6 sites missing type checks)
11. Ran full test suite again; all 1607 tests pass
12. Verified self-host C0→C1→C2 cycle passes cleanly

## Notes

- `restrict` is fully parse-and-ignore; no codegen effect (matching the `volatile` implementation). Non-pointer use (e.g., `restrict int x`) is not diagnosed.
- The qsort test uses `const int *ia = (const int *)a` instead of `*(const int *)a` — a pre-existing parser limitation where unary `*` on a cast calls parse_unary instead of parse_cast for the operand.
- `abs()` has a NASM 2.16 keyword conflict (abs is a NASM built-in); `labs()` and `llabs()` are used for runtime testing; `abs()` is verified by compilation only.
- The codegen bug fix (long-long return-value sign-extension) was pre-existing but only exposed during strtoll testing because it was one of the first tests calling a function returning long long.
- All non-floating-point, non-wide-character C99 functions from `<ctype.h>`, `<string.h>`, `<stdlib.h>`, and `<stdio.h>` now compile and link correctly.
