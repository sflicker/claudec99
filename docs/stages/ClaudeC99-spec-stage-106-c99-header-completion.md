# ClaudeC99 Stage 106 — C99 Header Completion (ctype, string, stdlib, stdio)

## Goal

Complete four stub headers to cover the full non-FP, non-wide-character C99
function set. Additionally add `restrict` keyword parsing (parse-and-ignore,
no codegen effect, matching the `volatile` pattern) so that the new
declarations — which use `restrict` on pointer parameters per C99 — compile
without error.

After this stage, a program using any non-floating-point, non-wide-character
function from these four headers will compile and link correctly against the
system libc.

---

## Background — current gaps (from gap analysis)

### `<ctype.h>` — missing 4 of 11 classifiers
Missing: `iscntrl` `isgraph` `isprint` `ispunct`

### `<string.h>` — missing 11 functions
Missing: `memmove` `memchr` `strcat` `strcoll` `strcspn` `strspn`
`strpbrk` `strstr` `strtok` `strerror` `strxfrm`

### `<stdlib.h>` — missing types, macros, ~20 functions
Missing types: `div_t` `ldiv_t` `lldiv_t`  
Missing macros: `EXIT_SUCCESS` `EXIT_FAILURE` `RAND_MAX` `MB_CUR_MAX`  
Missing functions: `abort` `atexit` `_Exit` `getenv` `system` `bsearch`
`qsort` `abs` `labs` `llabs` `div` `ldiv` `lldiv` `atoi` `atol` `atoll`
`strtoll` `strtoull` `rand` `srand`

### `<stdio.h>` — missing types, macros, ~25 functions
Missing type: `fpos_t`  
Missing macros: `BUFSIZ` `FOPEN_MAX` `FILENAME_MAX` `L_tmpnam` `TMP_MAX`
`_IOFBF` `_IOLBF` `_IONBF`  
Missing functions: `remove` `rename` `tmpfile` `tmpnam` `freopen` `fflush`
`setbuf` `setvbuf` `sprintf` `vsprintf` `scanf` `fscanf` `sscanf` `vscanf`
`vfscanf` `vsscanf` `fputc` `fputs` `getc` `getchar` `gets` `putc` `ungetc`
`fwrite` `fgetpos` `fsetpos` `rewind` `clearerr` `feof` `ferror` `perror`

### Minor fixes to other headers (small corrections, bundled here)
- `<stdbool.h>`: `__Bool_true_false_are_defined` → `__bool_true_false_are_defined`
- `<stddef.h>`: add `ptrdiff_t`
- `<limits.h>`: add `CHAR_MIN` `CHAR_MAX` `MB_LEN_MAX`

---

## C99 references

| Feature | C99 section |
|---------|-------------|
| `restrict` qualifier | §6.7.3 |
| `<ctype.h>` | §7.4 |
| `<string.h>` | §7.21 |
| `<stdlib.h>` | §7.20 |
| `<stdio.h>` | §7.19 |

---

## Task 0 — `restrict` keyword (`src/lexer.c`, `src/parser.c`)

`restrict` is a type qualifier that may only appear on pointer types (C99
§6.7.3). The compiler needs to tokenize and silently consume it; no codegen
effect is needed (same pattern as `volatile`).

### 0a. Lexer: add `TOKEN_RESTRICT` (`src/lexer.c`)

In the keyword recognition table, add an entry for `restrict` that emits
`TOKEN_RESTRICT`, parallel to the existing `TOKEN_CONST` and `TOKEN_VOLATILE`
entries.

### 0b. Parser: consume `TOKEN_RESTRICT` in pointer declarators (`src/parser.c`)

In `parse_declarator`, the loop that processes leading `*` characters already
handles `TOKEN_CONST` and `TOKEN_VOLATILE` after each star. Add a parallel
check for `TOKEN_RESTRICT` that consumes the token and discards it:

```c
/* After each '*': */
while (parser->current.type == TOKEN_CONST ||
       parser->current.type == TOKEN_VOLATILE ||
       parser->current.type == TOKEN_RESTRICT) {
    if (parser->current.type == TOKEN_CONST)
        pointer_is_const = 1;
    /* volatile and restrict: consume and ignore */
    parser->current = lexer_next_token(parser->lexer);
}
```

Apply the same change in every location where `TOKEN_CONST` / `TOKEN_VOLATILE`
are consumed in pointer position:

- `parse_declarator` — the leading-star loop
- `parse_type_name` — abstract declarator star loop
- `parse_parameter_declaration` — after parsing the base type, before the
  declarator (for patterns like `const char * restrict s`)

**Note:** `restrict` is only valid on pointer types in standard C99. The stub
implementation accepts it in any pointer position and silently discards it;
it does not diagnose `restrict int x` (non-pointer use). This is acceptable
for a parse-and-ignore implementation.

Also add `TOKEN_RESTRICT` to the `token_type_name()` / `--print-tokens`
display table with the string `"restrict"`.

---

## Task 1 — `<ctype.h>` — add 4 classifiers

Add after the existing `isxdigit` declaration:

```c
int iscntrl(int);
int isgraph(int);
int isprint(int);
int ispunct(int);
```

No ordering dependency; these are simple independent declarations.

---

## Task 2 — `<string.h>` — add 11 functions

Add after the existing `strrchr` declaration. Use `restrict` where C99
specifies it; note that `memmove` does **not** use `restrict` because it is
explicitly designed for overlapping regions.

```c
void *memmove(void *s1, const void *s2, size_t n);
void *memchr(const void *s, int c, size_t n);
char *strcat(char * restrict s1, const char * restrict s2);
size_t strcspn(const char *s1, const char *s2);
size_t strspn(const char *s1, const char *s2);
char *strpbrk(const char *s1, const char *s2);
char *strstr(const char *s1, const char *s2);
char *strtok(char * restrict s1, const char * restrict s2);
int   strcoll(const char *s1, const char *s2);
size_t strxfrm(char * restrict s1, const char * restrict s2, size_t n);
char *strerror(int errnum);
```

**Notes:**
- `strxfrm` writes at most `n` characters to `s1`. When `n == 0`, `s1` may
  be a null pointer (C99 §7.21.4.5); the declaration is correct as-is.
- `strtok` is inherently stateful (uses internal static state); callers must
  pass `NULL` as the first argument for subsequent calls on the same string.
- `strcoll` and `strxfrm` are locale-dependent; in the "C" locale they behave
  identically to `strcmp` and `strncpy` respectively.

---

## Task 3 — `<stdlib.h>` — add types, macros, and 21 functions

### 3a. Add struct typedefs (before function declarations)

```c
typedef struct { int       quot; int       rem; } div_t;
typedef struct { long      quot; long      rem; } ldiv_t;
typedef struct { long long quot; long long rem; } lldiv_t;
```

The field order (`quot` then `rem`) matches the System V AMD64 ABI layout
used by glibc on Linux, so `div()` / `ldiv()` / `lldiv()` return values
are correctly interpreted by the caller.

### 3b. Add macros (after the `#include <stddef.h>` line)

```c
#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1
#define RAND_MAX     32767
#define MB_CUR_MAX   1
```

`RAND_MAX` must be at least 32767 per C99 §7.20.2.1; our value satisfies
the requirement and matches the minimum portable assumption.

`MB_CUR_MAX` is technically a locale-dependent expression in hosted C99, but
defining it as 1 is correct for the "C" locale (which is the only locale we
support).

### 3c. Add function declarations (after `strtoull`)

```c
/* process control */
void  abort(void);
int   atexit(void (*func)(void));
void  _Exit(int status);
int   system(const char *string);
char *getenv(const char *name);

/* random numbers */
int  rand(void);
void srand(unsigned int seed);

/* integer arithmetic */
int       abs(int j);
long      labs(long j);
long long llabs(long long j);
div_t     div(int numer, int denom);
ldiv_t    ldiv(long numer, long denom);
lldiv_t   lldiv(long long numer, long long denom);

/* string conversion */
int       atoi(const char *nptr);
long      atol(const char *nptr);
long long atoll(const char *nptr);
long long          strtoll(const char * restrict nptr,
                            char ** restrict endptr, int base);
unsigned long long strtoull(const char * restrict nptr,
                             char ** restrict endptr, int base);

/* searching and sorting */
void *bsearch(const void *key, const void *base,
              size_t nmemb, size_t size,
              int (*compar)(const void *, const void *));
void  qsort(void *base, size_t nmemb, size_t size,
            int (*compar)(const void *, const void *));
```

**Notes:**
- `atexit` takes a `void (*)(void)` function pointer; ClaudeC99 supports
  function-pointer parameters, so this declaration is handled correctly.
- `bsearch` and `qsort` also use function-pointer parameters in the same way.
- `_Exit` (with leading underscore) terminates without calling `atexit`
  handlers or flushing streams; it is a C99 standard function, not a POSIX
  extension.
- Floating-point conversions (`atof`, `strtod`, `strtof`, `strtold`) are
  deferred until floating-point support is added. Multi-byte/wide conversions
  (`mbstowcs`, `mbtowc`, `wcstombs`, `wctomb`) are deferred until wide
  character support is added.
- `rand()`/`srand()` are included because they are in C99 §7.20.2 and widely
  used; the system libc provides the implementation.

---

## Task 4 — `<stdio.h>` — add type, macros, and 31 functions

### 4a. Add `fpos_t` type

Add immediately after the `#include <stdarg.h>` line and before `typedef
struct FILE FILE`:

```c
typedef long fpos_t;
```

`fpos_t` is an opaque type in C99; `long` is sufficient for our purposes
since we do not implement `fgetpos`/`fsetpos` ourselves — the system libc
does, and its `fpos_t` may differ. Programs using `fpos_t` variables but
not storing into them from `fgetpos` will compile correctly. Programs that
call `fgetpos`/`fsetpos` will link against libc and use libc's layout, which
on Linux x86_64 is also a `long`-sized type.

### 4b. Add macros

Add after the `#define SEEK_END 2` block:

```c
#define BUFSIZ       8192
#define FOPEN_MAX    16
#define FILENAME_MAX 4096
#define L_tmpnam     20
#define TMP_MAX      238328

#define _IOFBF 0
#define _IOLBF 1
#define _IONBF 2
```

### 4c. Add function declarations

Add after the `int putchar(int);` line (before `#endif`):

```c
/* file operations */
int   remove(const char *filename);
int   rename(const char *old, const char *new);
FILE *tmpfile(void);
char *tmpnam(char *s);

/* open / control */
FILE *freopen(const char * restrict filename,
              const char * restrict mode, FILE * restrict stream);
int   fflush(FILE *stream);
void  setbuf(FILE * restrict stream, char * restrict buf);
int   setvbuf(FILE * restrict stream, char * restrict buf,
              int mode, size_t size);

/* formatted output */
int sprintf(char * restrict s, const char * restrict format, ...);
int vsprintf(char * restrict s, const char * restrict format, va_list arg);

/* formatted input */
int scanf(const char * restrict format, ...);
int fscanf(FILE * restrict stream, const char * restrict format, ...);
int sscanf(const char * restrict s, const char * restrict format, ...);
int vscanf(const char * restrict format, va_list arg);
int vfscanf(FILE * restrict stream, const char * restrict format, va_list arg);
int vsscanf(const char * restrict s, const char * restrict format, va_list arg);

/* character output */
int fputc(int c, FILE *stream);
int fputs(const char * restrict s, FILE * restrict stream);
int putc(int c, FILE *stream);

/* character input */
int   getc(FILE *stream);
int   getchar(void);
char *gets(char *s);
int   ungetc(int c, FILE *stream);

/* binary I/O */
size_t fwrite(const void * restrict ptr, size_t size, size_t nmemb,
              FILE * restrict stream);

/* positioning */
int   fgetpos(FILE * restrict stream, fpos_t * restrict pos);
int   fsetpos(FILE * restrict stream, const fpos_t *pos);
void  rewind(FILE *stream);

/* error */
void clearerr(FILE *stream);
int  feof(FILE *stream);
int  ferror(FILE *stream);
void perror(const char *s);
```

**Notes:**
- `scanf`/`fscanf`/`sscanf` and the `v`-variants are added as declarations
  only; the system libc provides the implementation. ClaudeC99 programs can
  call them and link correctly even though the compiler itself does not
  implement format-string parsing.
- `gets` is deprecated in C99 and removed in C11; it is included here for
  C99 completeness.
- `feof` and `ferror` are specified as functions in C99 §7.19.10, though
  many libc implementations also provide them as macros. Declaring them as
  functions is always valid.
- `fwrite` was already listed as a pending TODO in the checklist (stage 87
  note); this stage delivers it.

---

## Task 5 — Minor fixes to other headers

These are small corrections to headers outside the four primary targets,
bundled here since they were identified in the same gap analysis.

### 5a. `<stdbool.h>` — fix macro name typo

```c
/* Before: */
#define __Bool_true_false_are_defined 1

/* After: */
#define __bool_true_false_are_defined 1
```

C99 §7.16 specifies lowercase `b`.

### 5b. `<stddef.h>` — add `ptrdiff_t`

Add after `typedef unsigned long size_t;`:

```c
typedef long ptrdiff_t;
```

`ptrdiff_t` is the signed result type of pointer subtraction (C99 §6.5.6).
On LP64 Linux, `long` is the correct underlying type.

### 5c. `<limits.h>` — add `CHAR_MIN`, `CHAR_MAX`, `MB_LEN_MAX`

Add after `UCHAR_MAX`:

```c
/* Plain char is signed on x86_64 Linux (implementation-defined) */
#define CHAR_MIN SCHAR_MIN
#define CHAR_MAX SCHAR_MAX
#define MB_LEN_MAX 16
```

`MB_LEN_MAX` is the maximum number of bytes in a multibyte character in any
supported locale; 16 is a conservative upper bound (UTF-8 uses at most 4, but
older ISO 2022 encodings use more).

---

## Task 6 — Version bump

In `src/version.c`, change:

```c
#define VERSION_STAGE  "01050000"
```
to:
```c
#define VERSION_STAGE  "01060000"
```

---

## Implementation order

1. `src/lexer.c` — add `TOKEN_RESTRICT` keyword (Task 0a)
2. `src/parser.c` — consume `TOKEN_RESTRICT` in all pointer-qualifier positions (Task 0b)
3. `test/include/ctype.h` — add 4 classifiers (Task 1)
4. `test/include/string.h` — add 11 functions (Task 2)
5. `test/include/stdlib.h` — add types, macros, 21 functions (Task 3)
6. `test/include/stdio.h` — add type, macros, 31 functions (Task 4)
7. `test/include/stdbool.h`, `stddef.h`, `limits.h` — minor fixes (Task 5)
8. `src/version.c` — bump stage (Task 6)
9. Tests (see below)
10. Build, full test suite, self-host cycle

---

## Out of scope — do NOT do these in this stage

- **Floating-point functions** (`atof`, `strtod`, `strtof`, `strtold`,
  `<math.h>`, `<float.h>`, `<fenv.h>`, `<complex.h>`, `<tgmath.h>`) —
  deferred until FP support is added.
- **Wide character / multibyte functions** (`mbstowcs`, `mbtowc`, `wcstombs`,
  `wctomb`, `<wchar.h>`, `<wctype.h>`) — deferred.
- **`<inttypes.h>`** — `PRI*`/`SCN*` format macros for `<stdint.h>` types;
  deferred (no format-string parsing in the compiler).
- **`<assert.h>`** — small but separate; deferred.
- **`<iso646.h>`** — `and`/`or`/`not` etc.; deferred.
- **`<locale.h>`**, **`<signal.h>`** — deferred.
- **`<stdint.h>` limit/constant macros** (`INT8_MAX`, `INT8_C(v)`, etc.) —
  identified as a gap but not in the four headers the user specified; deferred.
- **Codegen effect for `restrict`** — `restrict` is parsed and discarded;
  alias-analysis optimizations are out of scope.
- **Diagnosing `restrict` on non-pointer types** — `restrict int x` is
  currently accepted; enforcing the "pointers only" rule is deferred.

---

## Tests

### Valid — `EXIT_SUCCESS` / `EXIT_FAILURE`

```c
/* test_stdlib_exit_codes__0.c */
#include <stdlib.h>
int main(void) { return EXIT_SUCCESS; }
```
Expected: exit 0.

```c
/* test_stdlib_exit_failure__1.c */
#include <stdlib.h>
int main(void) { return EXIT_FAILURE; }
```
Expected: exit 1.

### Valid — `abs` / `labs` / `llabs`

```c
/* test_stdlib_abs__0.c */
#include <stdlib.h>
int main(void) {
    if (abs(-5)    != 5)   return 1;
    if (labs(-5L)  != 5L)  return 2;
    if (llabs(-5LL) != 5LL) return 3;
    return 0;
}
```
Expected: exit 0.

### Valid — `atoi`

```c
/* test_stdlib_atoi__0.c */
#include <stdlib.h>
int main(void) {
    return (atoi("42") == 42 && atoi("-7") == -7) ? 0 : 1;
}
```
Expected: exit 0.

### Valid — `qsort`

```c
/* test_stdlib_qsort__0.c */
#include <stdlib.h>
#include <string.h>
static int cmp(const void *a, const void *b) {
    return *(const int *)a - *(const int *)b;
}
int main(void) {
    int arr[5] = {5, 3, 1, 4, 2};
    int expected[5] = {1, 2, 3, 4, 5};
    qsort(arr, 5, sizeof(int), cmp);
    return memcmp(arr, expected, sizeof(arr)) ? 1 : 0;
}
```
Expected: exit 0.

### Valid — `memmove` (overlapping)

```c
/* test_string_memmove__0.c */
#include <string.h>
int main(void) {
    char buf[8] = "abcde";
    memmove(buf + 1, buf, 4);   /* overlap: shift right by 1 */
    return (buf[0]=='a' && buf[1]=='a' && buf[2]=='b' && buf[3]=='c' && buf[4]=='d') ? 0 : 1;
}
```
Expected: exit 0.

### Valid — `strstr`

```c
/* test_string_strstr__0.c */
#include <string.h>
int main(void) {
    const char *s = "hello world";
    return (strstr(s, "world") == s + 6) ? 0 : 1;
}
```
Expected: exit 0.

### Valid — `strcat`

```c
/* test_string_strcat__0.c */
#include <string.h>
int main(void) {
    char buf[16] = "hello";
    strcat(buf, " world");
    return strcmp(buf, "hello world");
}
```
Expected: exit 0.

### Valid — `strtok`

```c
/* test_string_strtok__0.c */
#include <string.h>
int main(void) {
    char s[] = "one,two,three";
    char *tok = strtok(s, ",");
    if (!tok || strcmp(tok, "one") != 0) return 1;
    tok = strtok(0, ",");
    if (!tok || strcmp(tok, "two") != 0) return 1;
    tok = strtok(0, ",");
    if (!tok || strcmp(tok, "three") != 0) return 1;
    return strtok(0, ",") != 0;
}
```
Expected: exit 0.

### Valid — `sprintf`

```c
/* test_stdio_sprintf__0.c */
#include <stdio.h>
#include <string.h>
int main(void) {
    char buf[32];
    sprintf(buf, "%d-%s", 42, "ok");
    return strcmp(buf, "42-ok");
}
```
Expected: exit 0.

### Valid — ctype new classifiers

```c
/* test_ctype_new_classifiers__0.c */
#include <ctype.h>
int main(void) {
    if (!isprint('A'))  return 1;
    if (!ispunct('!'))  return 2;
    if (!isgraph('A'))  return 3;
    if ( iscntrl('A'))  return 4;
    if (!iscntrl('\t')) return 5;
    return 0;
}
```
Expected: exit 0.

### Valid — `restrict` in declarations compiles cleanly

```c
/* test_restrict_pointer_decl__0.c */
#include <string.h>
int main(void) {
    char dst[8];
    const char *src = "hello";
    /* memmove and strcpy both use restrict in their signatures */
    memmove(dst, src, 6);
    return strcmp(dst, "hello");
}
```
Expected: exit 0.

### Valid — `strtoll` / `strtoull`

```c
/* test_stdlib_strtoll__0.c */
#include <stdlib.h>
int main(void) {
    long long a = strtoll("9223372036854775807", 0, 10);
    unsigned long long b = strtoull("18446744073709551615", 0, 10);
    return (a == 9223372036854775807LL && b == 18446744073709551615ULL) ? 0 : 1;
}
```
Expected: exit 0.

---

## Docs (at stage close, after all tests pass)

- **`README.md`** — update "Through stage …" line; note that `restrict` is now
  parsed; refresh test totals.
- **`docs/grammar.md`** — add `restrict` to the type-qualifier section.
- Run the **`update-supplemental-docs`** skill.
- **`docs/self-compilation-report.md`** — record the stage-106 self-host run.
