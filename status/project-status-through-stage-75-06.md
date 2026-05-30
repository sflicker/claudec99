# ClaudeC99 Project Status — Through Stage 75-06

_As of 2026-05-29 (commit `59d40cc`)_

## Overview

ClaudeC99 is a from-scratch C99 subset compiler written in C, targeting
x86_64 Linux via NASM (Intel syntax). The compiler is built incrementally
through small, spec-driven stages — each stage is a self-contained
specification followed by a kickoff, implementation, and milestone /
transcript record. Output is single-file assembly that is assembled with
`nasm -f elf64` and linked with `gcc -no-pie` (so crt0 / libc are
available for declared libc calls such as `puts` and `printf`).

**Stages completed: 164** (stage-01 through stage-75-06).

## Build & Test

| Component                     | Count          |
|-------------------------------|----------------|
| Source files (`src/*.c`)      | 10             |
| Header files (`include/`)     | 10             |
| Total LOC (src + include)     | 11,577         |
| Valid runtime tests           | 745            |
| Invalid (compile-error) tests | 224            |
| Integration tests             | 71             |
| Print-AST golden tests        | 41             |
| Print-tokens golden tests     | 99             |
| Print-asm golden tests        | 21             |
| **Total tests**               | **1,201**      |
| Git commits                   | 546            |

All 1,201 tests pass with no regressions.

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

`test/include/` now provides a broad set of controlled stub headers so
that real-world-shaped programs preprocess, parse, and compile without
host system headers:

- **`stdio.h`** — opaque `FILE`, `EOF`, `fopen`, `fclose`, `fgetc`,
  `fgets`, `fprintf`, `snprintf`, `puts`, `printf`, and the variadic
  forwarding family `vfprintf` / `vprintf` / `vsnprintf`.
- **`stdlib.h`** — `malloc`, `free`, `exit`, `realloc`.
- **`string.h`** — `strlen`, `strcmp`, `strcpy`, `memcpy`, `memset`,
  `memcmp`, `strchr`.
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

### Statements
- `return <expr>;` and bare `return;`.
- `if` / `else`, `while`, `do … while`, `for`.
- `switch` / `case` / `default` (linear dispatch; nested switches OK).
- `break`, `continue`, `goto` + labeled statement.
- Compound block statements with lexical scoping and shadowing.
- Expression statements.

### Declarations & types
- Integer base types `char`/`short`/`int`/`long`, all unsigned variants,
  `long long` / `unsigned long long`, the `signed` keyword forms, and
  trailing-`int` forms.
- `_Bool` with value-normalization and integer promotion.
- Integer literal suffixes `U`/`L`/`UL`/`LL`/`ULL` with overflow-aware
  typing.
- `void` and generic `void *`.
- `const` qualifier on base-type scalars **and pointer-level `const`
  enforcement** — writes through a `const *`, reassignment of a const
  pointer, and const-discarding conversions are diagnosed (the last as a
  warning, or an error under `-Werror`).
- Pointer types of arbitrary depth; array types; multi-level subscript.
- Function pointer types in local, parameter, file-scope, static, and
  extern positions; indirect calls.
- Parenthesized and abstract declarators; array-to-pointer decay for
  array-typedef function parameters.
- Comma-separated init-declarator lists; brace and string initializers
  (local and file scope) with size inference.
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
- Integer, string, and character literals; variable references.
- All eleven assignment operators; chained assignment; comma operator;
  conditional (ternary) operator.
- Arithmetic, bitwise, shift, equality/relational, and logical operators
  (signed and unsigned aware, with short-circuit evaluation).
- Pointer arithmetic including difference (`long`); prefix/postfix
  `++` / `--`.
- Casts; integer promotions and UAC including LP64 rules.
- `sizeof(type)` and `sizeof expr` (operand not evaluated).
- Address-of, dereference (rvalue and lvalue), subscript, multi-level
  subscript, member access `.` / `->`.
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
  byte-by-byte struct copies; `.data`/`.bss` globals and block-scope
  statics with RIP-relative access.
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

## Stage-by-Stage Timeline (66–75)

Stages 01–65 are catalogued in
`project-status-through-stage-65.md`; new stages since:

| Stage      | Focus                                                         |
|------------|---------------------------------------------------------------|
| 66         | Pointer-level `const` compatibility hardening                 |
| 67-01      | Opaque `FILE` typedef and `EOF` constant                      |
| 67-02      | `fopen` / `fclose` / `fgetc`                                   |
| 67-03      | `fgets` (line input)                                          |
| 67-04      | `fprintf` (file output)                                       |
| 67-05      | `snprintf`                                                    |
| 68         | More than 6 call arguments (SysV stack-passing)               |
| 69         | Memory/string header functions (`realloc`, `memcpy`, …)       |
| 70         | Mini compiler-shaped integration test                         |
| 70-01      | Tooling: `--version`, `--max-errors`, parser error recovery   |
| 70-02      | Line/column tracking in tokens                                |
| 70-03      | `file:line:col:` prefixes on errors and warnings              |
| 71         | Block-scope `static` local variables                          |
| 72         | Named union support                                           |
| 73-01      | Anonymous struct/union type declarations                      |
| 74         | Header gap fill (`ctype.h`, `errno.h`, `time.h`, `setjmp.h`)  |
| 75-01      | Variadic function definitions and caller compatibility        |
| 75-02      | `stdarg.h` and `va_list` surface; array-param decay           |
| 75-03      | `__builtin_va_*` parsing and semantic recognition             |
| 75-04      | `va_start` codegen foundation (register save area)            |
| 75-05      | Additional `va_list` integration tests                        |
| **75-06**  | **`va_arg` for integer and pointer types (current)**          |

## Recently Shipped (Stages 66–75)

**Stage 66** added pointer-level `const` enforcement: writes through a
`const` pointer and const-pointer reassignment are errors, and
const-discarding conversions warn (or error under `-Werror`). An
`is_const` field on `Type` and `type_const_copy()` carry the qualifier
without mutating shared singletons.

**Stage 67 (01–05)** built out file I/O in the `stdio.h` stub: an opaque
`FILE` typedef and `EOF`, then `fopen`/`fclose`/`fgetc`, `fgets`,
`fprintf`, and `snprintf` — all declarative, requiring no compiler
changes.

**Stage 68** removed the hard-coded 6-argument limit and implemented full
System V AMD64 stack-passing for 7+ arguments on both the caller side
(16-byte alignment, right-to-left stack pushes, register-arg loads) and
the callee side (stack parameters copied from `[rbp + 16 + (i-6)*8]`
with sign/zero extension). Indirect calls follow the same strategy.

**Stage 69** added `realloc` (`stdlib.h`) and `memcpy`, `memset`,
`memcmp`, `strchr` (`string.h`) as stub declarations.

**Stage 70** shipped a realistic mini-compiler-shaped integration test,
then a run of tooling stages: **70-01** added a versioning scheme
(`--version`), `--max-errors=N` multi-error collection, and setjmp/longjmp
parser recovery (replacing ~220 `exit(1)` pairs with uniform
`compile_error()` calls); **70-02** added `line`/`col`/`file` source
positions to every token and a `[line:col]` print-tokens column;
**70-03** put `<file>:<line>:<col>:` prefixes on all parser errors and
warnings and wired `-Werror` to a global flag.

**Stage 71** implemented block-scope `static` local variables that
persist across calls, emitted to `.bss`/`.data` with RIP-relative
`[rel Lstatic_func_N]` addressing.

**Stage 72** added named unions — tag table, `TYPE_UNION`, max-member
sizing, `.`/`->` access, whole-union assignment, first-member brace
initialization, and globals.

**Stage 73-01** added anonymous struct/union type declarations, each
allocating a unique `Type*` so that structurally identical anonymous
types remain distinct under assignment.

**Stage 74** filled header gaps with `ctype.h`, `errno.h`, `time.h`, and
`setjmp.h` stubs, plus a codegen fix allowing the null constant `0` as an
argument to a pointer parameter (so `time(0)` type-checks).

**Stage 75 (01–06)** delivered callee-side variadic support end to end:
**75-01** allowed variadic function definitions with unnamed fixed
parameters; **75-02** shipped `stdarg.h` with the `va_list` typedef and
`va_*` macros (plus array-to-pointer parameter decay); **75-03** parsed
and semantically validated the `__builtin_va_*` intrinsics; **75-04**
generated the variadic prologue register save area and `va_start`
initialization (and added `vfprintf`/`vprintf`/`vsnprintf`); **75-05**
added integration tests for 0/6/10-argument variadic calls; and
**75-06** implemented `va_arg` for GP-class types (int/long/long
long/pointer) with both register-save and overflow-stack paths per the
SysV AMD64 ABI.

## Out of Scope (Not Yet Implemented)

- Floating-point types (`float`, `double`) and all FP
  arithmetic/literals/conversions.
- `for`-loop initializer declarations (`for (int i = 0; …)`).
- `va_arg` for floating-point and struct-by-value types; `va_copy`
  codegen (still a no-op stub).
- Octal (`'\123'`) and hex (`'\x41'`) character/string escapes.
- Multi-character constants (`'ab'`); wide-character literals.
- Struct by-value parameters and return values (hidden-pointer ABI — a
  prerequisite for self-hosting).
- Bit-fields, flexible array members, compound literals.
- `volatile` and `restrict` qualifiers.
- `typedef enum`; enum constant expressions beyond integer/character
  literals; enum constants (and other non-literal constant expressions)
  as `switch`/`case` labels.
- Subscripting and postfix `++`/`--` on member-access (and other
  non-identifier) operands; compound assignment to non-identifier
  lvalues. (These parser limitations are surfaced by the
  self-compilation diagnostic; see `docs/self-compilation-report.md`.)
- Functions returning function pointers; `__attribute__` specifiers.
- `#pragma` (including `#pragma once`); `#elifdef` / `#elifndef`.
- GNU variadic macro extensions (`__VA_OPT__`, named variadic args,
  comma deletion).
- Object-file emission, linking, and multi-file `.o` compilation.

## Architecture

```
src/
├── preprocessor.c       Two-phase preprocessor (splicing, comments, directives, macros)
├── lexer.c              Tokenizer (with line/col source positions)
├── parser.c             Recursive-descent parser, builds AST; setjmp/longjmp recovery
├── ast.c                AST node lifecycle helpers
├── ast_pretty_printer.c --print-ast renderer
├── type.c               Type system (singletons + heap pointer chains)
├── codegen.c            Single-pass walker → NASM Intel-syntax asm
├── compiler.c           Driver (command-line parsing, -D/-I/--sysroot, --version, --max-errors)
├── version.c            Build/version identifier
└── util.c               Misc helpers; compile_error_at / compile_warning_at
```

The grammar is documented in `docs/grammar.md` and is updated
alongside any stage that touches it.

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
