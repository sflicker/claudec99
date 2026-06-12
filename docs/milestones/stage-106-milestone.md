# Milestone Summary

## Stage 106 — C99 Header Completion: Complete

Stage 106 ships C99 header completion (`restrict` keyword parsing, plus comprehensive stub completions for `ctype.h`, `string.h`, `stdlib.h`, and `stdio.h`).

- **Tokenizer**: Added `TOKEN_RESTRICT` keyword to lexer; `restrict` is recognized and tokenized like `const` and `volatile`.
- **Parser**: Extended all pointer-qualifier positions to consume `TOKEN_RESTRICT` (parse-and-ignore): leading-star loop in `parse_declarator`, abstract declarator star loop in `parse_type_name`, and qualifier-handling in `parse_parameter_declaration`. No semantic or codegen effect.
- **Stub headers**: 
  - `<ctype.h>`: added `iscntrl`, `isgraph`, `isprint`, `ispunct`.
  - `<string.h>`: added `memmove`, `memchr`, `strcat`, `strcoll`, `strcspn`, `strspn`, `strpbrk`, `strstr`, `strtok`, `strerror`, `strxfrm` (with `restrict` per C99).
  - `<stdlib.h>`: added `div_t`/`ldiv_t`/`lldiv_t` typedefs; `EXIT_SUCCESS`/`EXIT_FAILURE`/`RAND_MAX`/`MB_CUR_MAX` macros; 21 functions (`abort`, `atexit`, `_Exit`, `system`, `getenv`, `rand`, `srand`, `abs`, `labs`, `llabs`, `div`, `ldiv`, `lldiv`, `atoi`, `atol`, `atoll`, `strtoll`, `strtoull`, `bsearch`, `qsort`).
  - `<stdio.h>`: added `fpos_t` typedef; `BUFSIZ`/`FOPEN_MAX`/`FILENAME_MAX`/`L_tmpnam`/`TMP_MAX`/`_IOFBF`/`_IOLBF`/`_IONBF` macros; 31 functions (file operations, I/O control, formatted I/O, character I/O, binary I/O, positioning, error handling).
  - `<stdbool.h>`, `<stddef.h>`, `<limits.h>`: minor fixes (`__bool_true_false_are_defined`, `ptrdiff_t`, `CHAR_MIN`/`CHAR_MAX`/`MB_LEN_MAX`).
- **Codegen fix**: Fixed a pre-existing bug where `TYPE_LONG_LONG` and `TYPE_UNSIGNED_LONG_LONG` were missing from six `rhs_is_long` checks in code generation, causing long-long function return values to be incorrectly sign-extended via `movsxd`. Bug was exposed during `strtoll` testing and fixed by adding the missing type checks.
- **Tests**: 13 new valid tests covering EXIT codes, `abs`/`labs`/`llabs`, `atoi`, `qsort`, `strtoll`/`strtoull`, `memmove`, `strstr`, `strcat`, `strtok`, `sprintf`, ctype classifiers, and restrict parsing. Full suite: 1607 tests pass (922 valid, 255 invalid, 86 integration, 50 print-AST, 100 print-tokens, 21 print-asm, 165 unit).
- **Docs**: Updated version to 01060000; self-host C0→C1→C2 cycle passed cleanly with no source changes needed.
- **Notes**: `restrict` is parse-and-ignore with no codegen effect (pattern matched to `volatile`). `abs()` has a NASM 2.16 keyword conflict; `labs()`/`llabs()` used for runtime testing. `qsort` test uses const-cast form instead of unary dereference (pre-existing parser limitation). All non-floating-point, non-wide-character C99 functions from these four headers now compile and link correctly.
