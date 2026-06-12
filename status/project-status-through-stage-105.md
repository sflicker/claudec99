# ClaudeC99 Project Status — Through Stage 105

_As of 2026-06-12 (commit `d2732c6`)_

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

**Stage 100** extends constant expressions to **file-scope initializers**:
integer-typed file-scope globals can now be initialized with full compile-time
integer constant expressions (arithmetic, bitwise, shift, unary, and `sizeof(type-name)`)
instead of literal-only.

**Stage 101** adds **block-scope static aggregates**: `static` arrays, structs,
and unions are now supported at block scope (previously only scalars/pointers).
Each static variable carries an initializer via a new `init_node` field.

**Stage 102** completes **static aggregate coverage**: designated initializers
(`[index]` and `.member`) now work in block-scope static arrays and file-scope
global arrays; multidimensional static arrays are fully supported.

**Stage 103** extends constant expressions to **block-scope static scalars**:
static scalar variables can now be initialized with the full constant-expression
evaluator (`eval_const_init`) instead of literal-only, enabling expressions like
`static int x = 1 + 2 * 3;` and `static int y = (1 << 4) | 3;`.

**Stage 104** completes the **constant-expression evaluator**: both
`eval_const_expr` (token-stream, used by case labels, enumerator values, array
designators, and file-scope initializers) and `eval_const_init` (AST-based, used
by block-scope static scalar initializers) now support the full C99 integer
constant expression operator set — relational (`<`, `<=`, `>`, `>=`), equality
(`==`, `!=`), logical-AND (`&&`), logical-OR (`||`), and the ternary conditional
(`?:`). Stage 104 also fixes a pre-existing additive/shift precedence bug. No
new tokens, AST node types, or grammar rules.

**Stage 105** completes the **C99 preprocessor feature set**: `#pragma`
directives (unknown pragmas silently ignored; `#pragma once` fully supported),
the `_Pragma("string")` operator, `#line` directive (overrides `__LINE__` and
`__FILE__` expansions), null directives (`#` alone on a line), and the `__func__`
predefined identifier (C99 §6.4.2.2). No new AST node types; `__func__` is
synthesized as an `AST_STRING_LITERAL` identical to a regular string literal.

**Stages completed: 212** (stage-01 through stage-105, including substages).

## Build & Test

| Component                     | Count          |
|-------------------------------|----------------|
| Source files (`src/*.c`)      | 12             |
| Header files (`include/`)     | 13             |
| Total LOC (src + include)     | 15,232         |
| Valid runtime tests           | 914            |
| Invalid (compile-error) tests | 256            |
| Integration tests             | 88             |
| Print-AST golden tests        | 50             |
| Print-tokens golden tests     | 100            |
| Print-asm golden tests        | 21             |
| Unit tests                    | 165            |
| **Total tests**               | **1,594**      |
| Git commits                   | 844            |

All 1,594 tests pass with no regressions — under the gcc-built compiler and
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
- **Preprocessor conditional expressions**: full evaluator for
  `#if` / `#elif` supporting integer literals, `defined(NAME)`,
  object-like macro expansion, undefined-identifier-as-0, unary
  `!` / `-` / `+` / `~`, parentheses, `==` / `!=` / `<` / `<=` /
  `>` / `>=`, `&&` / `||`, `+` / `-` / `*` / `/` / `%`, `&` / `^` /
  `|`, `<<` / `>>`.
- **`#error message`**.
- **`#pragma`**: unknown pragmas silently ignored; **`#pragma once`**
  records the file path in `MacroTable` and causes subsequent includes
  of that file to return an empty result — stage 105.
- **`_Pragma("string")`**: C99 §6.10.9 operator; string is destringized
  and applied as a pragma; `_Pragma("once")` acts as `#pragma once`;
  all other strings silently ignored; expands to nothing — stage 105.
- **`#line digit ["filename"]`**: overrides `__LINE__` and `__FILE__`
  expansions for subsequent source text — stage 105.
- **Null directive**: bare `#` on a line silently ignored (C99 §6.10.7)
  — stage 105.
- **Predefined macros — standard**: `__FILE__`, `__LINE__`, `__DATE__`,
  `__TIME__`, `__STDC__`, `__STDC_VERSION__`, `__STDC_HOSTED__`,
  `__CLAUDEC99__`.
- **Predefined macros — ABI/target**: `__x86_64__`, `__linux__`,
  `__unix__`, `__LP64__`, `_LP64`, `__CHAR_BIT__`, and the full
  `__SIZEOF_*__` family (including `__SIZEOF_LONG_LONG__`).
- **Command-line flags**: `-DNAME` / `-DNAME=VALUE`; `-I<dir>` /
  `-I <dir>`; `--sysroot=<dir>`.
- **Stub system headers** in `test/include/` (see below).

### Stub system headers

`test/include/` provides a broad set of controlled stub headers so that
real-world-shaped programs preprocess, parse, and compile without host
system headers:

- **`stdio.h`** — opaque `FILE`, `EOF`, `fopen`, `fclose`, `fgetc`,
  `fgets`, `fprintf`, `snprintf`, `puts`, `printf`, `putchar`, the
  variadic forwarding family `vfprintf` / `vprintf` / `vsnprintf`, the
  standard streams `stdin` / `stdout` / `stderr` (extern `FILE *`), and
  file-position / read stubs `fseek` / `ftell` / `fread` with
  `SEEK_SET` / `SEEK_CUR` / `SEEK_END`.
- **`stdlib.h`** — `malloc`, `calloc`, `free`, `realloc`, `exit`,
  `strtol`, `strtoul`.
- **`string.h`** — `strlen`, `strcmp`, `strcpy`, `memcpy`, `memset`,
  `memcmp`, `strchr`, plus `strncat` / `strncmp` / `strncpy` /
  `strrchr` and related additions.
- **`stddef.h`** — `NULL`, `size_t`.
- **`stdint.h`** — exact-width, pointer-size, fast, and least integer
  typedefs.
- **`limits.h`** — full set including `LLONG_MIN`, `LLONG_MAX`,
  `ULLONG_MAX`.
- **`stdbool.h`** — `bool`, `true`, `false`.
- **`ctype.h`** — character classification (`isalpha`, `isdigit`,
  `isspace`, …).
- **`errno.h`** — `errno`, `ERANGE`, `EINVAL`, etc.
- **`time.h`** — `time_t` and time functions.
- **`setjmp.h`** — non-local jump support.
- **`stdarg.h`** — `va_list` typedef and the `va_start` / `va_end` /
  `va_copy` / `va_arg` macros (expanding to `__builtin_va_*`).

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
  integer/character literals, enum constants, arithmetic, bitwise, shift,
  unary, relational, equality, logical-AND/OR, and ternary (stages 77, 99, 104).
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
  RIP-relative addressing); **integer globals accept full compile-time
  constant expressions for initializers** (stages 100, 104).
- Storage-class specifiers `extern` and `static` at file scope, **plus
  block-scope `static` local variables** (stage 71: scalars/pointers;
  stage 101: arrays/structs/unions; stages 103–104: full constant-expression
  initializers for scalars including relational, equality, logical, and ternary)
  that persist across calls and are emitted to `.bss` (uninitialized) or
  `.data` (initialized) with RIP-relative `[rel Lstatic_func_N]` addressing.
  **Designated initializers work in static arrays** (stage 102);
  **multidimensional static arrays supported** (stage 102).
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
  Enumerator values accept **full C99 integer constant expressions**
  (arithmetic, shift, bitwise, relational, equality, logical, ternary,
  unary `~ !`, parentheses, and references to previously-defined enum
  constants) — stages 99, 104.
  Forward-declared enum tags (`typedef enum Status Status;` before the
  body) are supported — stage 99.

### Integer constant expressions (stages 77, 99–104)

A unified fifteen-level evaluator (`eval_const_expr`) is used for:
- **`case` labels**: `case 1 << 2:` (evaluates to 4), `case PERM_READ | PERM_WRITE:`, `case 2 > 1:`.
- **Enumerator values**: `FLAG_READ = 1 << 0`, `STEP = BASE + 5`, `ALL = ~0`,
  `POSITIVE = 1 > 0`, `LIMIT = DEBUG ? 10 : 100`.
- **Array designator indices**: `[2 + 1] = value`.
- **File-scope scalar initializers**: `int x = sizeof(long) == 8 ? 64 : 32;` (stages 100, 104).
- **Block-scope static scalar initializers**: `static int x = 1 && 1;` (stages 103, 104).

Operators supported (loosest to tightest): `?: || && | ^ & == != < <= > >=`
`<< >> + - * / %` (division-by-zero → compile error); unary `+ - ~ !`;
parenthesized sub-expressions; integer/character literals; enum constants
by name; `sizeof(type-name)` (stage 100).

### Designated initializers (C99 §6.7.8)
- **`.member = value`** member designators in struct brace initializers
  at both local and global scope.
- **`[index] = value`** array index designators at both local and global
  scope; the index must be a non-negative constant integer expression.
- **Both forms work in block-scope static arrays** (stage 102).
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
- `va_copy` is recognized but its codegen is still a no-op stub.

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
- **`__func__`** predefined identifier (C99 §6.4.2.2): within a function
  body, expands to a string literal containing the function name; use
  outside a function body is a compile-time error — stage 105.

### Code generation
- Single-pass walk from AST to NASM Intel-syntax assembly.
- Integer promotions, UAC, and LP64-aware conversions at every binary op.
- `movzx`/`movsx` sized loads; `div`/`idiv`; `shr`/`sar`.
- String literals in `.rodata`; struct/union member addressing and block
  struct copies (`rep movsb`); `.data`/`.bss` globals.
- Struct/union by-value marshalling; designated and compound literal init.
- Per-file codegen teardown; dynamic Vec/StrBuf storage (no fixed caps).

### Tooling & diagnostics
- `--version`, `--print-ast`, `--print-tokens`, `--print-asm` modes.
- `--max-errors=N` multiple-error collection with parser recovery.
- Multi-mode build workflow (`build.sh`): `--mode=normal` / `bootstrap` /
  `fallback` / `self-host` (C0→C1→C2 with test runs and checkpoint commits).
- Strict ISO C99 source; CMake build with `-Wall -Wextra -pedantic`.

## Stage-by-Stage Timeline (76–105)

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
| 83         | Project source converted to strict ISO C99                    |
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
| 99         | `typedef enum` completion — const expr evaluator + forward refs |
| 100        | File-scope constant expressions for integer globals            |
| 101        | Block-scope static arrays, structs, and unions                |
| 102        | Complete static aggregate coverage (designated-init, 2D)      |
| 103        | Block-scope static scalar constant-expression initializers     |
| 104        | Complete constant-expression evaluator — relational, equality, logical, ternary |
| **105**    | **C99 preprocessor completion — `#pragma`/once, `_Pragma`, `#line`, null directive, `__func__` (current)** |

## Recently Shipped (Stages 101–105)

**Stage 101 — Block-Scope Static Aggregates.**

Block-scope `static` declarations now support array, struct, and union types
(previously only scalars and pointers). A new `init_node` field on
`LocalStaticVar` carries initializers for aggregate statics. RIP-relative
addressing applies to array decay, subscript, and member access. No parser
changes; codegen-only.

**Stage 102 — Complete Static Aggregate Coverage.**

Designated initializers (`[index]` and `.member`) now work in block-scope
static arrays and file-scope global arrays. Multidimensional static arrays
are fully supported (both locally and globally). Codegen uses a slots array
to track designated positions. No parser changes.

**Stage 103 — Block-Scope Static Scalar Constant-Expression Initializers.**

Block-scope `static` scalar variables now accept the full `eval_const_init`
constant-expression evaluator instead of a 3-case literal-only check. This
enables `static int x = 1 + 2 * 3;`, `static int y = (1 << 4) | 3;`, etc.
Codegen-only change; supports arithmetic, bitwise, shift, unary, and
`sizeof(type-name)` operators.

**Stage 104 — Complete Constant-Expression Evaluator.**

Both `eval_const_expr` (token-stream evaluator in `src/parser.c`) and
`eval_const_init` (AST evaluator in `src/codegen.c`) are extended to support
the full C99 integer constant expression operator set: relational (`<`, `<=`,
`>`, `>=`), equality (`==`, `!=`), logical-AND (`&&`), logical-OR (`||`), and
the ternary conditional (`?:`). Stage 104 also fixes a pre-existing
additive/shift precedence bug. No parser, AST, or grammar changes.

**Stage 105 — C99 Preprocessor Completion.**

Five C99 preprocessing features that were previously missing or rejected:

1. **`#pragma`**: The directive handler is added immediately before the
   "unsupported directive" fallthrough. Unknown pragmas are silently consumed.
   `#pragma once` records the current source file path in a new
   `MacroTable.once_paths` dynamic array; `preprocess_file` returns an empty
   string on any subsequent include of a path already in that list. This
   prevents double-inclusion without requiring header guards.

2. **`_Pragma("string")`**: The C99 operator form. In the identifier expansion
   section of `preprocess_internal`, `_Pragma` is recognized before the general
   macro lookup. The argument string is destringized (backslash-escapes resolved),
   and if the content is `"once"` the current file is added to `once_paths`.
   All other pragma strings are silently ignored. `_Pragma(...)` expands to
   nothing (zero preprocessing tokens).

3. **`#line digit-sequence ["filename"]`**: Overrides the `current_line` counter
   (set to `new_line - 1` so the newline ending the directive increments it to
   the intended value) and, when a quoted filename follows, sets a new
   heap-allocated `current_file_override` string. Subsequent `__LINE__` and
   `__FILE__` macro expansions respect these overrides. The `__FILE__` handler
   is updated to prefer `current_file_override` over `source_path` when non-NULL.

4. **Null directive** (C99 §6.10.7): A `#` with only optional whitespace before
   the newline is now a `continue` rather than falling through to the
   "unsupported directive" error.

5. **`__func__`** (C99 §6.4.2.2): Recognized in `parse_primary` before the
   `__builtin_va_*` check. The `Parser` struct gains a `const char *current_func_name`
   field (NULL at file scope, set to `d.name` for the duration of each function
   body). Inside a function body, `__func__` synthesizes an `AST_STRING_LITERAL`
   with `node->value = lexer_store_bytes(fname, len)`, `byte_length = len`, and
   `full_type = type_array(type_char(), len + 1)` — identical to a string literal
   in type and storage. Use outside a function body is a `PARSER_ERROR`.

10 new tests added (7 valid, 1 invalid, 2 integration). All 1594 tests pass.
Self-host C0→C1→C2 passes cleanly.

## Out of Scope (Not Yet Implemented)

- Floating-point types (`float`, `double`) and all FP
  arithmetic/literals/conversions.
- `va_arg` for floating-point and struct-by-value types; `va_copy`
  codegen (still a no-op stub).
- Multi-character constants (`'ab'`); wide-character literals.
- Compound literals at file scope.
- Bit-fields, flexible array members.
- `volatile` code-generation semantics (currently parsed and tracked only);
  `restrict` qualifier.
- Chained designators (`.a.b`, `.arr[0]`); designated union init for
  non-first members.
- Functions returning function pointers; pointer-to-array declarators
  (`(*p)[10]`); old-style (K&R) function definitions; `__attribute__`
  specifiers.
- `#elifdef` / `#elifndef` (C23).
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
