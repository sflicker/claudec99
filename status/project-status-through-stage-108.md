# ClaudeC99 Project Status — Through Stage 108

_As of 2026-06-13 (commit 14e5a07)_

## Overview

ClaudeC99 is a from-scratch C99 subset compiler written in C, targeting
x86_64 Linux via NASM (Intel syntax). The compiler is built incrementally
through small, spec-driven stages — each stage is a self-contained
specification followed by a kickoff, implementation, and milestone /
transcript record. Output is single-file assembly that is assembled with
`nasm -f elf64` and linked with `gcc -no-pie` (so crt0 / libc are
available for declared libc calls such as `puts` and `printf`).

Since stage 83 the compiler's **own source compiles as strict ISO C99**
(`-std=c99 -pedantic-errors`), and **as of stage 92 the compiler is fully
self-hosting**: C0 (the gcc-built compiler) compiles its own source to
produce C1, and C1 compiles itself to produce C2 — both C1 and C2 pass the
entire test suite, confirming bootstrap stability (see
`docs/self-compilation-report.md`). Stage 93 adds a multi-mode build
workflow (`build.sh`); stage 94 adds a repeatable `--mode=self-host` C0→C1→C2
cycle. Stages 95-02 through 95-12 are an internal **dynamic-storage refactor**:
new `Vec` and `StrBuf` containers replace the parser/codegen fixed-capacity
tables, and all token/AST/parser/codegen name and label text moves from
`char[MAX_NAME_LEN]` buffers into `const char *` pointers backed by a
lexer-owned string pool.

**Stage 96** adds **multi-file compilation**: the driver (`src/compiler.c`)
now accepts one or more positional source-file arguments, compiling each
through a fresh Lexer/Parser/CodeGen/AST cycle with full per-file heap
teardown (`parser_free`, `codegen_free`, `lexer_free`, `ast_free`).

**Stage 97** adds **designated initializers** (C99 §6.7.8): both
`.member = value` member designators (for struct/union initializers) and
`[index] = value` array index designators.

**Stage 98** adds **compound literals** (C99 §6.5.2.5): the `(type-name){ initializer-list }`
syntax creates unnamed temporary objects on the stack. Both struct/union and
array compound literals are supported, along with scalar compound literals
(`(int){ 7 }`).

**Stage 99** completes the **`typedef enum`** feature: (1) enumerator values
now accept full integer constant expressions (arithmetic, bitwise `& ^ |`,
shift `<< >>`, multiplicative `* / %`, unary `~ !`, parenthesized
sub-expressions, and references to previously-defined enum constants) instead
of bare integer/character literals only; (2) forward-declared enum tags
(`typedef enum Status Status;` before the body) are now accepted as
placeholders that return `type_int()`. Internally, the old three-function
`eval_case_const_*` chain is replaced by a nine-level recursive descent
evaluator (`eval_const_expr`) that is also used for `case` labels and array
designator indices.

**Stage 100** adds **file-scope constant expressions**: integer-typed global variable 
initializers now accept full compile-time constant expressions (arithmetic, bitwise, shift, 
unary operators) plus `sizeof(type-name)`, using the shared `eval_const_expr` evaluator.

**Stage 101** lifts the block-scope static aggregate restriction: `static int arr[8]` and 
`static struct Point p = {1, 2}` are now fully supported with RIP-relative `.bss`/`.data` 
emission.

**Stage 102** completes static aggregate coverage: block-scope static designated-initializer 
arrays, struct/union element types, and 2D arrays all work.

**Stage 103** extends block-scope static scalar initializers to accept full compile-time constant 
expressions, matching the expressiveness of `case` labels, enum values, and file-scope globals.

**Stage 104** completes the **integer constant-expression evaluator**: both the parser-level 
`eval_const_expr` and the codegen-level `eval_const_init` now support the full C99 operator set 
— relational (`< <= > >=`), equality (`== !=`), logical AND/OR (`&&`/`||`), and the ternary 
operator. A pre-existing additive/shift precedence bug is also fixed.

**Stage 105** adds **C99 preprocessor completion**: `#pragma` (unknown pragmas silently ignored; 
`#pragma once` tracks seen files), `_Pragma("str")` operator, `#line` directives (with optional 
filename override for `__FILE__`), the null directive, and the `__func__` predefined identifier.

**Stage 106** completes **C99 stub headers** for `<ctype.h>`, `<string.h>`, `<stdlib.h>`, and 
`<stdio.h>`, adds the `restrict` type qualifier (parse-and-ignore), and fixes a pre-existing 
codegen bug where `TYPE_LONG_LONG`/`TYPE_UNSIGNED_LONG_LONG` were missing from `rhs_is_long` 
checks in 8 code paths, causing `long long` return values to be truncated via `movsxd`.

**Stage 107** closes three independent C99 gaps: (1) the **`inline` function-specifier** is
now recognized and consumed (parse-and-ignore — same pattern as `restrict` and `volatile`),
enabling `inline`, `static inline`, and `extern inline` function declarations without a
compile error; (2) a **`<assert.h>` stub** is added to `test/include/`, providing an
NDEBUG-aware `assert` macro using preprocessor stringification (`#expr`), `__FILE__`,
`__LINE__`, `fprintf`, and `abort`; (3) **`va_copy` codegen** is implemented — the silent
no-op is replaced by three 8-byte `rax` moves that copy the 24-byte SysV AMD64 `va_list`
struct from the source to the destination. A preprocessor fix (`__FILE__`/`__LINE__`
expansion inside function-like macro bodies via static globals in `src/preprocessor.c`)
was required to make the `assert` macro work correctly.

**Stage 108** adds **`#elifdef` and `#elifndef`** preprocessor directives (C23 §6.10.1 /
GCC/Clang extension). `#elifdef NAME` is equivalent to `#elif defined(NAME)` and
`#elifndef NAME` is equivalent to `#elif !defined(NAME)`. Both new branches are inserted
before `#elif` in `preprocess_internal`'s directive chain and correctly update
`cond_stack` state even inside inactive regions, ensuring proper nesting. The change is
entirely confined to `src/preprocessor.c`; no lexer, parser, AST, or codegen changes
were needed.

**Stages completed: 215** (stage-01 through stage-108, including substages).

## Build & Test

| Component                     | Count          |
|-------------------------------|----------------|
| Source files (`src/*.c`)      | 12             |
| Header files (`include/`)     | 13             |
| Total LOC (src + include)     | 15,390         |
| Valid runtime tests           | 941            |
| Invalid (compile-error) tests | 256            |
| Integration tests             | 88             |
| Print-AST golden tests        | 50             |
| Print-tokens golden tests     | 100            |
| Print-asm golden tests        | 21             |
| Unit tests                    | 165            |
| **Total tests**               | **1,621**      |
| Git commits                   | 863            |

All 1,621 tests pass with no regressions — under the gcc-built compiler and
under the self-compiled C1/C2 bootstrap binaries alike.

## Language Features Implemented

### Preprocessing

The compiler ships a full preprocessing pipeline that runs before
lexical analysis.

- **Line splicing**: backslash-newline sequences are joined before any
  other processing.
- **Comment removal**: `//` and `/* */` comments are stripped (replaced
  with a single space to preserve token separation).
- **`#include "file.h"`**: local quoted includes resolved relative to the
  including file's directory; recursion depth limited to 64.
- **`#include <file.h>`**: angle-bracket includes resolved via `-I`
  search paths in command-line order.
- **Object-like macros**: `#define NAME replacement`; identifier
  expansion in ordinary source; compatible redefinitions accepted,
  incompatible ones rejected.
- **Function-like macros**: `#define NAME(params) replacement`; exact
  argument count validated; arguments pre-expanded before substitution;
  nested macro invocations recursively expanded.
- **Variadic macros**: `#define M(...)` and `#define M(x, ...)`;
  `__VA_ARGS__` substituted with extra comma-separated arguments.
- **Stringification** (`#param`) and **token pasting** (`##`).
- **`#undef NAME`**.
- **Conditional compilation**: `#ifdef` / `#ifndef` / `#if` / `#elif` /
  `#else` / `#endif`; nesting up to 64 levels; first-true-wins
  `#elif` chains.
- **`#elifdef NAME`** (stage 108): branch-transition directive equivalent to
  `#elif defined(NAME)` (C23 §6.10.1 / GCC/Clang extension). Correctly
  updates `cond_stack` state even in inactive regions.
- **`#elifndef NAME`** (stage 108): branch-transition directive equivalent to
  `#elif !defined(NAME)` (C23 §6.10.1 / GCC/Clang extension).
- **Preprocessor conditional expressions**: full evaluator for
  `#if` / `#elif` supporting integer literals, `defined(NAME)`,
  object-like macro expansion, undefined-identifier-as-0, unary
  `!` / `-` / `+` / `~`, parentheses, `==` / `!=` / `<` / `<=` /
  `>` / `>=`, `&&` / `||`, `+` / `-` / `*` / `/` / `%`, `&` / `^` /
  `|`, `<<` / `>>`.
- **`#error message`**.
- **`#pragma` (unknown pragmas silently ignored; `#pragma once` tracks 
  canonical paths)**.
- **`_Pragma("str")` operator** (recognized in identifier expansion; 
  `"once"` triggers `#pragma once`; others ignored).
- **`#line N ["file"]` directives** (sets current line; optional filename 
  overrides `__FILE__` expansion).
- **Null directive** (bare `#` followed by whitespace+newline → silently 
  ignored per C99 §6.10.7).
- **Predefined macros — standard**: `__FILE__`, `__LINE__`, `__DATE__`,
  `__TIME__`, `__STDC__`, `__STDC_VERSION__`, `__STDC_HOSTED__`,
  `__CLAUDEC99__`.
- **Predefined macros — ABI/target**: `__x86_64__`, `__linux__`,
  `__unix__`, `__LP64__`, `_LP64`, `__CHAR_BIT__`, and the full
  `__SIZEOF_*__` family (including `__SIZEOF_LONG_LONG__`).
- **`__func__` predefined identifier** (synthesized `char []` containing 
  the enclosing function name; error at file scope).
- **`__FILE__`/`__LINE__` in function-like macro bodies** (stage 107):
  `expand_macros_text` now resolves these predefined macros using
  invocation-site context, so macros like `assert` that embed `__FILE__`
  and `__LINE__` in their body produce correct file/line output.
- **Command-line flags**: `-DNAME` / `-DNAME=VALUE`; `-I<dir>` /
  `-I <dir>`; `--sysroot=<dir>`.
- **Stub system headers** in `test/include/` (see below).

### Stub system headers

`test/include/` provides a broad set of controlled stub headers so that
real-world-shaped programs preprocess, parse, and compile without host
system headers:

- **`assert.h`** — NDEBUG-aware `assert` macro: when `NDEBUG` is defined,
  `assert(expr)` expands to `((void)0)`; otherwise it evaluates the
  expression and, if false, prints an `"assertion failed: …"` message to
  stderr (using `#expr` stringification, `__FILE__`, and `__LINE__`) then
  calls `abort()` (stage 107).
- **`stdio.h`** — complete stub: opaque `FILE`, `fpos_t`, `EOF`, `stdin`/`stdout`/`stderr`; 
  macros `BUFSIZ`, `FOPEN_MAX`, `FILENAME_MAX`, `L_tmpnam`, `TMP_MAX`, `_IOFBF`/`_IOLBF`/`_IONBF`, 
  `SEEK_*`; functions `fopen`/`fclose`/`fgetc`/`fgets`/`fprintf`/`snprintf`/`puts`/`printf`/`putchar`, 
  variadic-forwarding family `vfprintf`/`vprintf`/`vsnprintf`, `fseek`/`ftell`/`fread`/`fwrite`, 
  and 31 additional functions (`remove`, `rename`, `tmpfile`, `tmpnam`, `freopen`, `fflush`, 
  `setbuf`, `setvbuf`, `sprintf`, `vsprintf`, `scanf`, `fscanf`, `sscanf`, `vscanf`, `vfscanf`, 
  `vsscanf`, `fputc`, `fputs`, `putc`, `getc`, `getchar`, `gets`, `ungetc`, `fgetpos`, `fsetpos`, 
  `rewind`, `clearerr`, `feof`, `ferror`, `perror`).
- **`stdlib.h`** — complete stub: `malloc`, `calloc`, `free`, `realloc`, `exit`, `strtol`, `strtoul`; 
  macros `EXIT_SUCCESS`/`EXIT_FAILURE`/`RAND_MAX`/`MB_CUR_MAX`; typedefs `div_t`/`ldiv_t`/`lldiv_t`; 
  functions `abort`, `atexit`, `_Exit`, `system`, `getenv`, `rand`, `srand`, `abs`, `labs`, `llabs`, 
  `div`, `ldiv`, `lldiv`, `atoi`, `atol`, `atoll`, `strtoll`, `strtoull`, `bsearch`, `qsort`.
- **`string.h`** — complete stub: `strlen`, `strcmp`, `strcpy`, `memcpy`, `memset`, `memcmp`, 
  `strchr`; plus `strncat`/`strncmp`/`strncpy`/`strrchr`; plus `memmove`, `memchr`, `strcat`, 
  `strcoll`, `strcspn`, `strspn`, `strpbrk`, `strstr`, `strtok`, `strerror`, `strxfrm` 
  (with `restrict` qualifiers per C99).
- **`stddef.h`** — `NULL`, `size_t`, `ptrdiff_t`.
- **`stdint.h`** — exact-width, pointer-size, fast, and least integer typedefs.
- **`limits.h`** — full set including `LLONG_MIN`/`LLONG_MAX`/`ULLONG_MAX`, `CHAR_MIN`/`CHAR_MAX`, 
  `MB_LEN_MAX`.
- **`stdbool.h`** — `bool`, `true`, `false`, `__bool_true_false_are_defined`.
- **`ctype.h`** — character classification: `isalpha`, `isdigit`, `isspace`, `isupper`, `islower`, 
  `isalnum`, `isxdigit`, `iscntrl`, `isgraph`, `isprint`, `ispunct`.
- **`errno.h`** — `errno`, `ERANGE`, `EINVAL`, etc.
- **`time.h`** — `time_t` and time functions.
- **`setjmp.h`** — non-local jump support.
- **`stdarg.h`** — `va_list` typedef and the `va_start`/`va_end`/`va_copy`/`va_arg` macros.

### Core program shape
- Translation unit with one or more external declarations.
- Function definitions and forward declarations (return-type and
  parameter-type compatibility checking across redeclarations).
- Multiple functions per translation unit; recursive calls.
- Variadic function **declarations, external calls, and definitions**;
  callee-side `<stdarg.h>` access (see Variadic functions below).
- **Struct/union arguments and return values passed by value** per the
  System V AMD64 ABI (see Struct/union by value below).
- `main` entry point; SysV AMD64 calling convention.
- Calls into libc emitted via NASM `extern`, resolved by `gcc -no-pie`.
- `extern`-declared **objects** (e.g. `extern FILE *stdout;`) register an
  `is_extern` global and emit an `extern` directive instead of `.bss`/
  `.data` storage; non-static file-scope definitions emit a NASM `global`
  directive so they are visible across the bootstrap link.
- **Multiple source files per invocation**: `ccompiler file1.c file2.c …`
  compiles each file independently through a fresh Lexer/Parser/CodeGen/AST
  pipeline with full per-file heap teardown — stage 96.

### Statements
- `return <expr>;` and bare `return;`.
- `if` / `else`, `while`, `do … while`, `for` — including **C99
  `for`-loop initializer declarations** (`for (int i = 0; …)`) scoped to
  the loop.
- `switch` / `case` / `default` (linear dispatch; nested switches OK).
  **`case` labels accept full compile-time integer constant expressions** —
  integer/character literals, enum constants, and all of `* / % << >> + - & ^ |`
  with unary `~ !`, parenthesized sub-expressions, relational `< <= > >=`, 
  equality `== !=`, and logical AND/OR `&&` `||` and ternary `?:` 
  (stage 77; extended stage 99; further extended stage 104).
- `break`, `continue`, `goto` + labeled statement.
- Compound block statements with lexical scoping and shadowing.
- Expression statements.

### Declarations & types
- Integer base types `char`/`short`/`int`/`long`, all unsigned variants,
  `long long` / `unsigned long long`, the `signed` keyword forms, and
  trailing-`int` forms.
- `_Bool` with value-normalization and integer promotion.
- Integer literal forms: decimal and **`0x`/`0X` hexadecimal** literals,
  with suffixes `U`/`L`/`UL`/`LL`/`ULL` and overflow-aware typing.
- `void` and generic `void *`.
- `const` qualifier on base-type scalars, **pointer-level `const`
  enforcement**, and **`const` in struct/union members and type-name
  contexts** (`sizeof`, casts, `va_arg`). Writes through a `const *`,
  reassignment of a const pointer, assignment to a `const` member
  (directly, via subscript, or through a pointer to a const-qualified
  object), and const-discarding conversions are diagnosed.
- **Minimal `volatile` handling**: the qualifier is tokenized, parsed at
  every position `const` is accepted, and tracked on `Type` /
  `StructField`, but has no code-generation effect yet.
- **`restrict` qualifier** (stage 106): tokenized and parsed at every 
  pointer-qualifier position (`parse_declarator`, `parse_type_name`, 
  `parse_parameter_declaration`); silently discarded with no semantic or 
  code-generation effect — same pattern as `volatile`.
- **`inline` function-specifier** (stage 107): tokenized (`TOKEN_INLINE`)
  and consumed in `parse_declaration_specifiers` with no semantic or
  code-generation effect — same parse-and-ignore pattern as `restrict` and
  `volatile`. Enables `inline int f()`, `static inline int f()`, and
  `extern inline int f()` without a compile error.
- Pointer types of arbitrary depth; array types; **multidimensional
  arrays** (declarations, indexing, member access, and `sizeof`, up to
  `MAX_ARRAY_DIMS = 8`); multi-level subscript.
- Function pointer types in local, parameter, file-scope, static, and
  extern positions; indirect calls.
- Parenthesized and abstract declarators; array-to-pointer decay for
  array-typedef function parameters and for **struct/union array members**
  in value contexts.
- Comma-separated init-declarator lists; brace and string initializers
  (local and file scope, including **char-array struct members from
  string literals**) with size inference; **designated initializers**
  (`.member = value` and `[index] = value`) at local and global scope
  for both arrays and structs — stage 97.
- **Compound literals** (`(type-name){ initializer-list }`) — unnamed
  temporary objects allocated on the stack (stage 98; see below).
- File-scope (global) variable declarations (`.bss` / `.data`,
  RIP-relative addressing).
- Storage-class specifiers `extern` and `static` at file scope, **plus
  block-scope `static` local variables** that persist across calls and
  are emitted to `.bss` (uninitialized) or `.data` (initialized) with
  RIP-relative `[rel Lstatic_func_N]` addressing (stage 101-103).
- `typedef` aliases for scalar / pointer / array / function-pointer and
  complete struct / union types; block-scope shadowing.
- Struct definitions, member access, brace initializers (including
  **designated member initializers** — `.field = value` — at local and
  global scope), whole-struct assignment/copy, pointer-to-struct mutation,
  nested structs, arrays of structs, typedef aliases, and incomplete
  forward declarations.
- **Named unions** (`union Tag { … }`): layout sized to the largest
  member, member access via `.` / `->`, nested types, whole-union
  assignment, first-member brace initialization, and globals.
- **Anonymous struct/union type declarations** without a tag: each
  definition allocates a fresh unique `Type*`; type identity is by
  pointer, so structurally identical anonymous types are distinct.
- **Enum declarations** with compile-time constant folding; `NULL`.
  Enumerator values accept full integer constant expressions
  (arithmetic, shift, bitwise, unary `~ !`, parentheses, relational, 
  equality, logical AND/OR, ternary operator, and references to 
  previously-defined enum constants) — stage 99 and extended stage 104.
  Forward-declared enum tags (`typedef enum Status Status;` before the
  body) are supported — stage 99.

### Integer constant expressions (stage 77; extended stages 99 & 104)

A unified thirteen-level evaluator (`eval_const_expr`) is used for:
- **`case` labels**: `case 1 << 2:` (evaluates to 4), `case PERM_READ | PERM_WRITE:`, 
  `case x > 5 ? 10 : 20:`.
- **Enumerator values**: `FLAG_READ = 1 << 0`, `STEP = BASE + 5`, `ALL = ~0`,
  `SMALL = (4 * 8)`, `FLAG = a < b ? 1 : 0`.
- **Array designator indices**: `[2 + 1] = value`.
- **File-scope and block-scope static initializers**: `int x = 1 + 2;`, `static int y = 3 * 4;`.

Operators supported (loosest to tightest): `?:` (ternary); `||` (logical OR); 
`&&` (logical AND); `|` (bitwise OR); `^` (bitwise XOR); `&` (bitwise AND); 
`==` `!=` (equality); `< <= > >=` (relational); `<< >>` (shift); `+ -` (additive); 
`* / %` (multiplicative); unary `+ - ~ !`; parenthesized sub-expressions; 
integer/character literals; enum constants by name; `sizeof(type-name)`.

Division-by-zero or modulo-by-zero produces a `PARSER_ERROR`.

### Designated initializers (C99 §6.7.8)
- **`.member = value`** member designators in struct brace initializers
  at both local and global scope.
- **`[index] = value`** array index designators at both local and global
  scope; the index must be a non-negative constant integer expression.
- Mixing designated and non-designated elements is supported.
- Chained designators and context mismatches are diagnosed.

### Compound literals (C99 §6.5.2.5)
- **Struct and union compound literals** on the stack.
- **Array compound literals** with explicit or inferred (from initializer)
  length; `[N]` index designators.
- **Scalar compound literals** (`(int){ 7 }`); modifiable lvalues.
- **Postfix chaining**: `.field` and `[index]` on compound literals.
- **Address-of**: `&(T){ … }` for all compound literal kinds.
- Compound literals at file scope are detected and rejected.

### Struct/union by value (System V AMD64 ABI)
- **Register-class aggregates** (≤16 bytes) travel in `rax`/`rdx` and
  integer argument registers.
- **Memory-class aggregates** (>16 bytes) travel through a hidden pointer
  (`rdi` for returns / `sret`).
- Whole-struct assignment and declaration-initialization accept struct
  rvalues from any target form (subscript, dot, arrow, deref).

### Variadic functions (`<stdarg.h>`)
- `va_start` / `va_end` / `va_arg` for GP-class types (int, long, pointers).
- **`va_copy`** copies the 24-byte SysV AMD64 `va_list` struct via three
  8-byte `rax` moves (stage 107; was a silent no-op stub).

### Expressions
- Integer (decimal/hex), string, and character literals; adjacent
  string-literal concatenation; hex/octal escapes; variable references.
- All eleven assignment operators on any modifiable lvalue.
- Arithmetic, bitwise, shift, equality/relational, and logical operators.
- Pointer arithmetic including difference (`long`).
- Casts; integer promotions and UAC.
- `sizeof(type)` and `sizeof expr` (operand not evaluated).
- Address-of on any addressable lvalue; dereference; subscript.
- Compound literals; conditional (ternary) operator; comma operator.
- Function calls with any number of arguments.

### Code generation
- Single-pass walk from AST to NASM Intel-syntax assembly.
- Integer promotions, UAC, and LP64-aware conversions at every binary op.
- `movzx`/`movsx` sized loads; `div`/`idiv`; `shr`/`sar`.
- String literals in `.rodata`; struct/union member addressing and block
  struct copies (`rep movsb`); `.data`/`.bss` globals.
- Struct/union by-value marshalling; designated and compound literal init.
- Per-file codegen teardown; dynamic Vec/StrBuf storage (no fixed caps).
- Correct `long long` / `unsigned long long` handling in return values and 
  assignments (stage 106: fixed missing `rhs_is_long` checks in 8 code paths).
- **`va_copy` codegen** (stage 107): three 8-byte `rax` moves copy the
  full 24-byte `va_list` struct from source to destination local variable.

### Tooling & diagnostics
- `--version`, `--print-ast`, `--print-tokens`, `--print-asm` modes.
- `--max-errors=N` multiple-error collection with parser recovery.
- Multi-mode build workflow (`build.sh`): `--mode=normal` / `bootstrap` /
  `fallback` / `self-host` (C0→C1→C2 with test runs and checkpoint commits).
- Strict ISO C99 source; CMake build with `-Wall -Wextra -pedantic`.

## Stage-by-Stage Timeline (76–108)

Stages 01–65 are catalogued in `project-status-through-stage-65.md`, and
66–75 in `project-status-through-stage-75-06.md`; new stages since:

| Stage      | Focus                                                         |
|------------|---------------------------------------------------------------|
| 76         | `for`-loop initializer declarations (C99)                     |
| 77         | Enum / constant-expression `case` labels                      |
| 78         | General postfix expression chaining                           |
| 79         | General-lvalue compound assignment                            |
| 80         | Prefix/postfix `++`/`--` on general lvalues                   |
| 81         | Header updates (`putchar`, `calloc`); `!` on pointers; limits |
| 82-01      | `const` qualifiers in struct/union members                    |
| 82-02      | const-qualified member lvalue (subscript) rules               |
| 82-03      | `const` in type-name contexts (sizeof/cast/va_arg)            |
| 82-04      | Minimal `volatile` handling                                   |
| 82-05      | const member / pointer-to-const-struct diagnostics            |
| 83         | Project source converted to strict ISO C99                   |
| 84         | Standard streams `stdin`/`stdout`/`stderr`; extern objects    |
| 84-02      | `stdlib.h` `exit()` declaration                               |
| 85         | Member-array to pointer decay; char-member string init        |
| 85-01      | `string.h` additional declarations                            |
| 86         | Multidimensional array support                                |
| 87         | `stdio.h` file-position / read stubs (`fseek`/`ftell`/`fread`)|
| 88         | Hex (`\xNN`) and octal (`\NNN`) character/string escapes      |
| 89         | Adjacent string-literal concatenation                         |
| 90         | Hexadecimal integer literals                                  |
| 91         | Address-of member lvalues                                     |
| 91-01      | Struct/union by-value parameters and returns (SysV ABI)       |
| 92         | Self-compilation validation — **full self-hosting achieved**  |
| 93         | Bootstrap build workflow; `VERSION_BUILTBY`                   |
| 94         | Self-host validation; `build.sh --mode=self-host` C0→C1→C2    |
| 95-01      | Fixed-capacity inventory (docs only)                          |
| 95-02      | `Vec` generic growable-array module                           |
| 95-03      | `StrBuf` dynamic string-buffer module                         |
| 95-04      | Low-risk static arrays → `Vec`                                |
| 95-05      | Medium-risk static arrays → `Vec`                             |
| 95-06      | High-risk static arrays → `Vec` (two latent bugs fixed)       |
| 95-07      | Remaining static usages → `Vec`; call-layout bounds guard     |
| 95-08      | Token text → pointer + length (255-byte string cap removed)   |
| 95-09      | `ASTNode.value` → `const char *`                              |
| 95-10      | parser.h name/tag fields → `const char *`                     |
| 95-11      | codegen.h name/label fields → `const char *`                  |
| 95-12      | `#if` unary buffer → `StrBuf`; switch labels → `Vec`          |
| 96         | Multi-file compilation; per-file teardown                     |
| 97         | Designated initializers (`.member =`, `[index] =`)            |
| 98         | Compound literals `(T){ … }` — struct, array, scalar          |
| 99         | `typedef enum` completion — const expr evaluator + forward refs|
| 100        | File-scope constant-expression initializers                   |
| 101        | Block-scope static arrays and structs                         |
| 102        | Complete static aggregate coverage (2D arrays, element types, designated init) |
| 103        | Block-scope static scalar constant-expression initializers    |
| 104        | Complete constant-expression evaluator (relational, equality, logical, ternary) |
| 105        | C99 preprocessor completion (`#pragma`/once, `_Pragma`, `#line`, `__func__`) |
| 106        | C99 header completion (`restrict` qualifier; ctype/string/stdlib/stdio stubs) |
| 107        | `inline` keyword, `<assert.h>` stub, `va_copy` codegen        |
| **108**    | **`#elifdef` / `#elifndef` preprocessor directives (current)**|

## Recently Shipped (Stages 106–108)

**Stage 106: C99 header completion and `restrict` qualifier.**

Stage 106 completes the set of C99 stub system headers. `<ctype.h>` now exports
the full character classification function set (`isalpha`, `isdigit`, `isspace`,
`isupper`, `islower`, `isalnum`, `isxdigit`, `iscntrl`, `isgraph`, `isprint`,
`ispunct`). `<string.h>` is expanded to include `memmove`, `memchr`, `strcat`,
`strcoll`, `strcspn`, `strspn`, `strpbrk`, `strstr`, `strtok`, `strerror`, and
`strxfrm`, all with `restrict` qualifiers per C99. `<stdlib.h>` gains `div_t`,
`ldiv_t`, and `lldiv_t` typedefs, `EXIT_SUCCESS`, `EXIT_FAILURE`, `RAND_MAX`,
and `MB_CUR_MAX` macros, and functions for memory management, numeric conversion,
searching, sorting, and pseudo-random generation. `<stdio.h>` is completed with
file-position and extended I/O functions (31 additional functions). The `restrict`
qualifier is now parsed at every pointer-qualifier position, with no semantic or
code-generation effect. As a bonus, stage 106 fixes a pre-existing codegen bug
where `TYPE_LONG_LONG` and `TYPE_UNSIGNED_LONG_LONG` were missing from eight
`rhs_is_long` checks in the code generator, causing `long long` values to be
incorrectly truncated via `movsxd`.

**Stage 107: `inline` keyword, `<assert.h>`, and `va_copy` codegen.**

Stage 107 closes three independent C99 gaps in a single batch.

The **`inline` function-specifier** (`TOKEN_INLINE`) is tokenized and consumed
by a new branch in `parse_declaration_specifiers`, with no semantic or codegen
effect — the same parse-and-ignore pattern used for `restrict` and `volatile`.
This enables `inline`, `static inline`, and `extern inline` function
declarations to compile without error, unblocking code that follows the common
C99 idiom of placing utility functions in headers.

A **`<assert.h>` stub** is added to `test/include/`. The stub provides an
NDEBUG-aware `assert` macro: when `NDEBUG` is defined the macro expands to
`((void)0)`; otherwise it checks the expression and on failure prints
`"assertion failed: <expr>, file <path>, line <N>"` to `stderr` before calling
`abort()` (exit status 134 = 128 + SIGABRT). Implementing this stub required a
preprocessor fix: `expand_macros_text` (which handles function-like macro body
rescan) did not previously expand `__FILE__` or `__LINE__`. Two file-scope static
globals (`g_expand_source_path`, `g_expand_current_line`) are now set by
`preprocess_internal` before each call to `expand_macros_text`, giving the
helper access to the invocation-site file and line context.

**`va_copy` codegen** replaces the silent no-op stub with three 8-byte `rax`
register moves that copy the 24-byte SysV AMD64 `va_list` struct (comprising
`gp_offset` [bytes 0–3], `fp_offset` [4–7], `overflow_arg_area` [8–15], and
`reg_save_area` [16–23]) from the source stack slot to the destination stack
slot. Both operands must be local variables resolved by `codegen_find_var`.

**Stage 108: `#elifdef` / `#elifndef` preprocessor directives.**

Stage 108 adds two C23 §6.10.1 branch-transition directives that are widely
used in system headers and real-world C code.

`#elifdef NAME` is equivalent to `#elif defined(NAME)`: when the enclosing
conditional frame is eligible for a new branch (`parent_emitting &&
!branch_taken`), the macro table is searched for `NAME` and `cond_val` is set
to 1 if the macro exists, 0 otherwise.

`#elifndef NAME` is equivalent to `#elif !defined(NAME)`: same logic but
`cond_val = (macro_find(...) == NULL)`.

Both branches are inserted immediately before the `#elif` branch in
`preprocess_internal`'s directive dispatch, consistent with the existing
`#ifdef`-before-`#if` ordering convention. Both are placed in the
pre-`emitting`-check region of the directive chain (the same zone as `#elif`,
`#else`, and `#endif`), so they correctly update `cond_stack` state even when
the surrounding block is inactive — this is the key requirement for proper
nesting and skip semantics.

The directives were standardised in C23 and are accepted as a GCC/Clang
extension in C99 mode. The change is entirely confined to `src/preprocessor.c`;
no lexer, parser, AST, or codegen changes were required.

## Out of Scope (Not Yet Implemented)

- Floating-point types (`float`, `double`) and all FP
  arithmetic/literals/conversions.
- `va_arg` for floating-point and struct-by-value types.
- Multi-character constants (`'ab'`); wide-character literals.
- Compound literals at file scope.
- Bit-fields, flexible array members.
- `volatile` code-generation semantics (currently parsed and tracked only).
- Chained designators (`.a.b`, `.arr[0]`); designated union init for
  non-first members.
- Functions returning function pointers; pointer-to-array declarators
  (`(*p)[10]`); old-style (K&R) function definitions; `__attribute__`
  specifiers.
- GNU variadic macro extensions (`__VA_OPT__`, named variadic args,
  comma deletion).
- Object-file (`.o`) emission and separate linking; header-only precompilation.

## Architecture

```
src/
├── preprocessor.c       Two-phase preprocessor (splicing, comments, directives, macros)
├── lexer.c              Tokenizer (line/col positions; hex literals; hex/octal escapes)
├── parser.c             Recursive-descent parser, builds AST; setjmp/longjmp recovery
├── ast.c                AST node lifecycle helpers (dynamic children array; ast_clone)
├── ast_pretty_printer.c --print-ast renderer
├── type.c               Type system (singletons + heap pointer chains; const/volatile copies)
├── codegen.c            Single-pass walker → NASM Intel-syntax asm; SysV struct-by-value ABI
├── compiler.c           Driver (multi-file loop; compile_one_file; per-file teardown)
├── version.c            Build/version identifier (VERSION_BUILTBY)
├── vec.c                Generic growable-array container (stage 95-02)
├── strbuf.c             Dynamic character/string buffer (stage 95-03)
└── util.c               Misc helpers; compile_error_at / compile_warning_at; reset_error_state
```

The grammar is documented in `docs/grammar.md`, the parser call hierarchy
in `docs/other/stage-108-parse-tree.md`, and the feature checklist in
`docs/outlines/checklist.md` — each updated alongside any stage that
touches it. The bootstrap workflow is driven by `build.sh`; the
self-compilation findings are recorded in `docs/self-compilation-report.md`.

## Process

Each stage produces, in order:
1. **Spec** in `docs/stages/`.
2. **Kickoff** in `docs/kickoffs/` — summary, change list, and
   spec-issue callouts before code is written.
3. **Implementation** committed in a single stage-scoped commit.
4. **Milestone summary** in `docs/milestones/`.
5. **Transcript** in `docs/sessions/` following
   `transcript-format.md`.

Tests live next to the runners in `test/valid`, `test/invalid`,
`test/print_ast`, `test/print_tokens`, `test/print_asm`, and
`test/integration/`, each driven by a `run_tests.sh` script.
