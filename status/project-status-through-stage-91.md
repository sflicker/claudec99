# ClaudeC99 Project Status — Through Stage 91

_As of 2026-06-03 (commit `587d861`)_

## Overview

ClaudeC99 is a from-scratch C99 subset compiler written in C, targeting
x86_64 Linux via NASM (Intel syntax). The compiler is built incrementally
through small, spec-driven stages — each stage is a self-contained
specification followed by a kickoff, implementation, and milestone /
transcript record. Output is single-file assembly that is assembled with
`nasm -f elf64` and linked with `gcc -no-pie` (so crt0 / libc are
available for declared libc calls such as `puts` and `printf`).

Since stage 83 the compiler's **own source compiles as strict ISO C99**
(`-std=c99 -pedantic-errors`), and as of stage 91 the **entire `src/`
tree self-compiles** under the project's own built compiler — every
module passes (see `docs/self-compilation-report.md`).

**Stages completed: 186** (stage-01 through stage-91).

## Build & Test

| Component                     | Count          |
|-------------------------------|----------------|
| Source files (`src/*.c`)      | 10             |
| Header files (`include/`)     | 11             |
| Total LOC (src + include)     | 12,719         |
| Valid runtime tests           | 819            |
| Invalid (compile-error) tests | 237            |
| Integration tests             | 82             |
| Print-AST golden tests        | 43             |
| Print-tokens golden tests     | 100            |
| Print-asm golden tests        | 21             |
| **Total tests**               | **1,302**      |
| Git commits                   | 630            |

All 1,302 tests pass with no regressions.

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
- **`stdlib.h`** — `malloc`, `calloc`, `free`, `realloc`, `exit`.
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
- `main` entry point; SysV AMD64 calling convention.
- Calls into libc emitted via NASM `extern`, resolved by `gcc -no-pie`.
- `extern`-declared **objects** (e.g. `extern FILE *stdout;`) register an
  `is_extern` global and emit an `extern` directive instead of `.bss`/
  `.data` storage.

### Statements
- `return <expr>;` and bare `return;`.
- `if` / `else`, `while`, `do … while`, `for` — including **C99
  `for`-loop initializer declarations** (`for (int i = 0; …)`) scoped to
  the loop.
- `switch` / `case` / `default` (linear dispatch; nested switches OK).
  **`case` labels accept compile-time integer constant expressions** —
  integer/character literals, enum constants, and unary/binary `+` / `-`
  over those.
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
  object), and const-discarding conversions are diagnosed (the last as a
  warning, or an error under `-Werror`).
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
  string literals**) with size inference.
- File-scope (global) variable declarations (`.bss` / `.data`,
  RIP-relative addressing).
- Storage-class specifiers `extern` and `static` at file scope, **plus
  block-scope `static` local variables** that persist across calls and
  are emitted to `.bss` (uninitialized) or `.data` (initialized) with
  RIP-relative `[rel Lstatic_func_N]` addressing.
- `typedef` aliases for scalar / pointer / array / function-pointer and
  complete struct / union types; block-scope shadowing.
- Struct definitions, member access, brace initializers, whole-struct
  assignment/copy, pointer-to-struct mutation, nested structs, arrays of
  structs, typedef aliases, and incomplete forward declarations.
- **Named unions** (`union Tag { … }`): layout sized to the largest
  member, member access via `.` / `->`, nested types, whole-union
  assignment, first-member brace initialization, and globals.
- **Anonymous struct/union type declarations** without a tag: each
  definition allocates a fresh unique `Type*`; type identity is by
  pointer, so structurally identical anonymous types are distinct.
- Enum declarations with compile-time constant folding; `NULL`.

### Variadic functions (`<stdarg.h>`)
- Variadic function **definitions** with unnamed fixed parameters; caller
  rule enforces `actual_arg_count >= fixed_param_count`.
- `va_start` / `va_end` parsed, validated, and code-generated: a 304-byte
  register save area is allocated for variadic functions, the prologue
  saves `rdi`–`r9`, and `va_start` initializes the `va_list` fields
  (`gp_offset`, `fp_offset`, `overflow_arg_area`, `reg_save_area`).
- `va_arg` for GP-class types (`int`, `unsigned int`, `long`,
  `unsigned long`, `long long`, `unsigned long long`, and pointers) via
  the full SysV AMD64 sequence: `gp_offset` range check, register-save
  path, and overflow-stack path, with type-appropriate 4- or 8-byte
  loads. Small promoted types (`char`/`short`/`_Bool`), aggregates by
  value, arrays, and `void` are rejected as `va_arg` types.
- `va_copy` is recognized but its codegen is still a no-op stub;
  `va_arg` for floating-point and struct-by-value types is future work.

### Expressions
- Integer (decimal/hex), string, and character literals; **adjacent
  string-literal concatenation**; **hex (`\xNN`) and octal (`\NNN`)
  escape sequences** in character and string literals; variable
  references.
- All eleven assignment operators on **any modifiable lvalue** (bare
  identifiers, member access, subscripts, dereferences, and chains);
  chained assignment; comma operator; conditional (ternary) operator.
- Arithmetic, bitwise, shift, equality/relational, and logical operators
  (signed and unsigned aware, with short-circuit evaluation); logical
  `!` accepts pointer operands.
- Pointer arithmetic including difference (`long`); prefix/postfix
  `++` / `--` on **any modifiable lvalue**.
- Casts; integer promotions and UAC including LP64 rules.
- `sizeof(type)` and `sizeof expr` (operand not evaluated), including
  multidimensional array types.
- **Address-of (`&`) on any addressable lvalue** — variables, subscripts,
  and member/arrow access (`&s.member`, `&p->member`, `&arr[i].member`);
  dereference (rvalue and lvalue); subscript; multi-level subscript;
  member access `.` / `->`.
- Function calls with **any number of arguments** (≤6 in registers, 7+
  via SysV stack-passing); variadic calls; indirect calls; `void *`
  implicit conversions; the null constant `0` accepted as a pointer
  argument.

### Code generation
- Single-pass walk from AST to NASM Intel-syntax assembly; per-function
  stack frames with natural alignment.
- Integer promotions, UAC, and LP64-aware conversions at every binary op.
- `movzx`/`movsx` sized loads; `div`/`idiv`; `shr`/`sar`; unsigned
  comparison instructions; `_Bool` normalization.
- String literals in `.rodata`; struct/union member addressing and
  byte-by-byte struct copies; `.data`/`.bss` globals, `extern` object
  directives, and block-scope statics with RIP-relative access.
- General-lvalue compound assignment (via `ast_clone()` desugaring) and
  general-lvalue `++`/`--` (`codegen_inc_dec_general()`); const-qualified
  and non-lvalue operands are rejected.
- Multidimensional array address computation (`get_subscript_element_type`,
  `emit_array_index_addr` scaling by inner array byte size).
- Function calls: register args for the first six, right-to-left stack
  pushes for the rest with 16-byte alignment; callee copies stack
  parameters from `[rbp + 16 + (i-6)*8]` with sign/zero extension;
  indirect calls via `call r10`; `xor eax, eax` before variadic calls.
- Variadic prologue register save area and `va_start`/`va_arg` lowering
  (see Variadic functions).

### Tooling & diagnostics
- `--version` reports a build identifier (`MM.mm.SSSSSSSS.BBBBB`; build
  number sourced from `git rev-list --count`).
- `--max-errors=N` collects multiple diagnostics before aborting
  (default 1 = exit on first error); parser recovers via setjmp/longjmp
  and `parser_sync()` to the next declaration boundary.
- Tokens carry `line` / `col` / `file` source positions; `--print-tokens`
  renders a `[line:col]` column.
- Parser errors and warnings carry `<file>:<line>:<col>:` prefixes via
  `compile_error_at` / `compile_warning_at`. (Codegen-side errors still
  print without a position prefix — AST nodes do not yet carry token
  info.)
- `-Werror` promotes warnings (e.g. const-discard) to errors via the
  global `g_warnings_are_errors`.
- `--print-ast`, `--print-tokens`, `--print-asm` rendering modes.
- The compiler's own source is strict ISO C99 (stage 83): `util_strdup()`
  replaces non-standard `strdup`, GNU `__attribute__` is wrapped in
  portable `CC_NORETURN` / `CC_PRINTF` macros, and CMake locks the build
  to `CMAKE_C_STANDARD 99` with `-Wall -Wextra -pedantic`.

## Stage-by-Stage Timeline (76–91)

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
| **91**     | **Address-of member lvalues (current)**                       |

## Recently Shipped (Stages 76–91)

**Self-compilation push (76–80, 91).** A run of parser/codegen stages
closed the gaps surfaced by the self-compilation diagnostic: **76** added
C99 `for`-init declarations scoped to the loop; **77** evaluated
constant-expression `case` labels (enum constants and `+`/`-` folding);
**78** let postfix operators chain on any base (`p->tokens[i].kind`) and
wrapped array-typed struct members in `TYPE_ARRAY`; **79** extended
compound assignment to general lvalues via an `ast_clone()` desugaring;
**80** extended prefix/postfix `++`/`--` to general lvalues with
`codegen_inc_dec_general()`; and **91** allowed unary `&` on member/arrow
lvalues (`&s.x`, `&sp->x`). As of stage 91 the entire `src/` tree
self-compiles.

**Stage 81** added `putchar` and `calloc` stubs, removed the restriction
blocking `!` on pointer operands (enabling `if (!p)`), and raised
`PARSER_MAX_FUNCTIONS` / `PARSER_MAX_GLOBALS` / `MAX_GLOBALS` /
`MAX_LOCALS` from 64 to 256.

**Stage 82 (01–05)** built out `const`/`volatile` qualifiers: **82-01**
parsed `const` struct/union members (pointer-to-const and const-scalar)
and rejected direct const-member assignment; **82-02** caught const
violations through subscript writes; **82-03** consumed `const` in
type-name contexts (`sizeof`, casts, `va_arg`); **82-04** added minimal
`volatile` handling (tokenized, parsed everywhere `const` is, tracked on
`Type`/`StructField`, no codegen effect); **82-05** added
`member_base_is_const()` to reject assignments to members of const objects
and through pointers to const structs.

**Stage 83** converted the compiler's own source to strict ISO C99
(passes `-std=c99 -pedantic-errors`): `util_strdup()` replaces `strdup`,
portable `CC_NORETURN`/`CC_PRINTF` macros replace GNU `__attribute__`, and
CMake locks to C99 with `-Wall -Wextra -pedantic`. Behavior-preserving.

**Stage 84 / 84-02** shipped the standard streams `stdin`/`stdout`/
`stderr` as `extern FILE *` declarations — which required teaching codegen
to register `extern` objects (`is_extern` global, `extern` directive, no
`.bss`/`.data` emission) — and added the `exit()` declaration to
`stdlib.h`.

**Stage 85 / 85-01** implemented array-to-pointer decay for struct/union
members in value contexts and char-array struct-member initialization from
string literals, then added `strncat`/`strncmp`/`strncpy`/`strrchr` and
related declarations to `string.h`.

**Stage 86** added full multidimensional array support — declarators and
abstract declarators collect up to `MAX_ARRAY_DIMS = 8` bracket suffixes
nested right-to-left by `build_array_type_from_dims`; indexing, member
access, decay, and `sizeof` all handle nested array types.

**Stage 87** added the `fseek`/`ftell`/`fread` file-position and read
stubs (with `SEEK_*` macros) to `stdio.h`.

**Stage 88** decoded hex (`\xNN`) and octal (`\NNN`, 1–3 digits) escape
sequences in character and string literals (with `\0` folded into the
octal branch). **Stage 89** concatenated adjacent string literals in
`parse_primary`, re-checking the `MAX_NAME_LEN` bound. **Stage 90** added
`0x`/`0X` hexadecimal integer literals, sharing all suffix/type logic with
the decimal path.

**Stage 91** extended unary `&` to member and arrow lvalues, dispatching
to `emit_member_addr`/`emit_arrow_addr` and setting the result type to
pointer-to-field — completing the last self-compilation parser gap.

## Out of Scope (Not Yet Implemented)

- Floating-point types (`float`, `double`) and all FP
  arithmetic/literals/conversions.
- `va_arg` for floating-point and struct-by-value types; `va_copy`
  codegen (still a no-op stub).
- Multi-character constants (`'ab'`); wide-character literals.
- Struct by-value parameters and return values (hidden-pointer ABI — a
  prerequisite for fuller self-hosting).
- Bit-fields, flexible array members, compound literals.
- `volatile` code-generation semantics (currently parsed and tracked only);
  `restrict` qualifier.
- `typedef enum`; enum constant expressions beyond integer/character
  literals and `+`/`-` folding.
- Designated initializers (`[idx] = …`, `.member = …`).
- Functions returning function pointers; pointer-to-array declarators
  (`(*p)[10]`); old-style (K&R) function definitions; `__attribute__`
  specifiers.
- `#pragma` (including `#pragma once`); `#elifdef` / `#elifndef`.
- GNU variadic macro extensions (`__VA_OPT__`, named variadic args,
  comma deletion).
- Object-file emission, linking, and multi-file `.o` compilation.

## Architecture

```
src/
├── preprocessor.c       Two-phase preprocessor (splicing, comments, directives, macros)
├── lexer.c              Tokenizer (line/col positions; hex literals; hex/octal escapes)
├── parser.c             Recursive-descent parser, builds AST; setjmp/longjmp recovery
├── ast.c                AST node lifecycle helpers (incl. ast_clone)
├── ast_pretty_printer.c --print-ast renderer
├── type.c               Type system (singletons + heap pointer chains; const/volatile copies)
├── codegen.c            Single-pass walker → NASM Intel-syntax asm
├── compiler.c           Driver (command-line parsing, -D/-I/--sysroot, --version, --max-errors)
├── version.c            Build/version identifier
└── util.c               Misc helpers; compile_error_at / compile_warning_at; util_strdup
```

The grammar is documented in `docs/grammar.md`, the parser call hierarchy
in `docs/other/stage-91-parse-tree.md`, and the feature checklist in
`docs/outlines/checklist.md` — each updated alongside any stage that
touches it.

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
