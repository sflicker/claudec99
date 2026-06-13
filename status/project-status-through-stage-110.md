# ClaudeC99 Project Status — Through Stage 110

_As of 2026-06-13 (commit `55e2ae4`)_

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
now accepts one or more positional source-file arguments.

**Stage 97** adds **designated initializers** (C99 §6.7.8): `.member = value`
member designators and `[index] = value` array index designators.

**Stage 98** adds **compound literals** (C99 §6.5.2.5): `(type-name){ initializer-list }`.

**Stage 99** completes the **`typedef enum`** feature: full integer constant
expression enumerator values and forward-declared enum tags.

**Stage 100** adds **file-scope constant expressions**: integer-typed global
variables accept full compile-time constant expressions (arithmetic, bitwise,
shift, unary, `sizeof(type-name)`) instead of requiring literals only.

**Stages 101/102** add **block-scope static aggregates**: `static int arr[8]`,
`static struct Point p`, and their initialized/designated variants now persist
across calls with RIP-relative addressing.

**Stage 103** extends **block-scope static scalar initializers** to accept full
constant expressions (arithmetic, bitwise, shift, unary, `sizeof`, relational).

**Stage 104** completes the **constant-expression evaluator**: both the
token-stream evaluator (`eval_const_expr` in `src/parser.c`) and the AST
evaluator (`eval_const_init` in `src/codegen.c`) now handle relational
(`<`/`<=`/`>`/`>=`), equality (`==`/`!=`), logical (`&&`/`||`), and ternary
(`?:`) operators. Also fixes the pre-existing additive/shift precedence inversion.

**Stage 106** adds **`restrict` keyword** parsing (parse-and-ignore) and
completes the stub headers for `ctype.h`, `string.h`, `stdlib.h`, and `stdio.h`.
Also fixes a pre-existing codegen bug where `TYPE_LONG_LONG` was missing from
`rhs_is_long` checks.

**Stage 107** adds **`inline` keyword** parsing (parse-and-ignore), a
`<assert.h>` stub header, and functional `va_copy` codegen (24-byte SysV
AMD64 `va_list` struct copy).

**Stage 108** adds **`#elifdef`/`#elifndef`** preprocessor branch-transition
directives (C23/GCC extension equivalents of `#elif defined(N)` and
`#elif !defined(N)`).

**Stage 109** adds **`float` and `double` as first-class scalar types**: 
`TOKEN_FLOAT`/`TOKEN_DOUBLE` keywords; `TOKEN_FLOAT_LITERAL`/`TOKEN_DOUBLE_LITERAL`
tokens; `AST_FLOAT_LITERAL` node; `TYPE_FLOAT` (size=4, align=4) and `TYPE_DOUBLE`
(size=8, align=8) types; local and global variable declarations (`.data`/`.bss`
with `dd`/`dq`/`resd`/`resq`); float/double struct/union members; implicit
float→double widening (`cvtss2sd`); and a per-TU `.rodata` FP literal pool
(`Lfc<N>: dd/dq` labels). FP arithmetic, comparisons, and function
parameters/return values are deferred to Stages 110–112.

**Stage 110** adds **float/double arithmetic, conversions, and casts**: all four
binary FP operations (`+`/`-`/`*`/`/`) via SSE2 (`addss`/`addsd`, `subss`/`subsd`,
`mulss`/`mulsd`, `divss`/`divsd`); FP unary minus via sign-bit XOR
(`xorps`/`xorpd` with 16-byte-aligned `.rodata` masks); explicit casts between
all scalar types (`cvtsi2ss`, `cvtsi2sd`, `cvtss2sd`, `cvtsd2ss`, `cvttss2si`,
`cvttsd2si`); Usual Arithmetic Conversions for mixed int/float/double expressions;
and fixes to `expr_result_type` which had silently misidentified FP local
variables as `TYPE_INT`/`TYPE_LONG`. No parser or AST changes. FP comparisons
and function parameters/return values remain for Stages 111/112.

**Stages completed: 223** (stage-01 through stage-110, including substages).

## Build & Test

| Component                     | Count          |
|-------------------------------|----------------|
| Source files (`src/*.c`)      | 12             |
| Header files (`include/`)     | 13             |
| Total LOC (src + include)     | 15,963         |
| Valid runtime tests           | 955            |
| Invalid (compile-error) tests | 256            |
| Integration tests             | 88             |
| Print-AST golden tests        | 50             |
| Print-tokens golden tests     | 100            |
| Print-asm golden tests        | 21             |
| Unit tests                    | 165            |
| **Total tests**               | **1,635**      |
| Git commits                   | 875            |

All 1,635 tests pass with no regressions — under the gcc-built compiler and
under the self-compiled C1/C2 bootstrap binaries alike.

## Language Features Implemented

### Preprocessing

The compiler ships a full preprocessing pipeline that runs before
lexical analysis.

- **Line splicing**: backslash-newline sequences are joined before any
  other processing.
- **Comment removal**: `//` and `/* */` comments are stripped.
- **`#include "file.h"`** and **`#include <file.h>`**: local and search-path includes.
- **Object-like and function-like macros**: `#define`/`#undef`; argument
  substitution; recursive expansion; variadic macros with `__VA_ARGS__`;
  stringification (`#param`); token pasting (`##`).
- **Conditional compilation**: `#ifdef`/`#ifndef`/`#if`/`#elif`/`#else`/`#endif`/
  **`#elifdef`/`#elifndef`** (C23/GCC extension — stage 108); nesting up to 64 levels.
- **Preprocessor conditional expressions**: full evaluator (`defined()`, macro
  expansion, integer literals, arithmetic, bitwise, shift, logical, comparison,
  unary operators, parentheses).
- **`#error message`**.
- **Predefined macros** — standard: `__FILE__`, `__LINE__`, `__DATE__`,
  `__TIME__`, `__STDC__`, `__STDC_VERSION__`, `__STDC_HOSTED__`, `__CLAUDEC99__`.
- **Predefined macros** — ABI/target: `__x86_64__`, `__linux__`, `__unix__`,
  `__LP64__`, `_LP64`, `__CHAR_BIT__`, and the full `__SIZEOF_*__` family.
- **Command-line flags**: `-DNAME` / `-DNAME=VALUE`; `-I<dir>` / `-I <dir>`; `--sysroot=<dir>`.
- **Stub system headers** in `test/include/` (see below).
- **`#pragma once`** and unknown pragmas silently ignored.

### Stub system headers

`test/include/` provides a broad set of controlled stub headers:

- **`stdio.h`** — opaque `FILE`, `EOF`, `fopen`, `fclose`, `fgetc`, `fgets`,
  `fprintf`, `snprintf`, `puts`, `printf`, `putchar`, variadic `vfprintf`/`vprintf`/
  `vsnprintf`, standard streams, file-position/read stubs, and 31 additional I/O
  functions (`feof`, `ferror`, `clearerr`, `remove`, `rename`, `tmpfile`, etc.).
- **`stdlib.h`** — `malloc`, `calloc`, `free`, `realloc`, `exit`, `strtol`,
  `strtoul`, `strtoll`, `strtoull`, `atoi`, `atol`, `atoll`, `abs`, `labs`, `llabs`,
  `div`, `ldiv`, `lldiv`, `div_t`/`ldiv_t`/`lldiv_t`, `qsort`, `bsearch`,
  `rand`, `srand`, `atexit`, `abort`, `_Exit`, `system`, `getenv`,
  `EXIT_SUCCESS`, `EXIT_FAILURE`, `RAND_MAX`.
- **`string.h`** — `strlen`, `strcmp`, `strcpy`, `memcpy`, `memset`, `memcmp`,
  `strchr`, `strncat`, `strncmp`, `strncpy`, `strrchr`, `memmove`, `memchr`,
  `strcat`, `strstr`, `strtok`, `strerror`, and others.
- **`stddef.h`** — `NULL`, `size_t`, `ptrdiff_t`.
- **`stdint.h`** — exact-width, pointer-size, fast, and least integer typedefs.
- **`limits.h`** — full set including `LLONG_MIN`, `LLONG_MAX`, `ULLONG_MAX`,
  `CHAR_MIN`, `CHAR_MAX`, `MB_LEN_MAX`.
- **`stdbool.h`** — `bool`, `true`, `false`, `__bool_true_false_are_defined`.
- **`ctype.h`** — full character classification (`isalpha`, `isdigit`, `isspace`,
  `iscntrl`, `isgraph`, `isprint`, `ispunct`, …).
- **`errno.h`** — `errno`, `ERANGE`, `EINVAL`, etc.
- **`time.h`** — `time_t` and time functions.
- **`setjmp.h`** — non-local jump support.
- **`stdarg.h`** — `va_list`, `va_start`, `va_end`, `va_copy`, `va_arg` macros.
- **`assert.h`** — NDEBUG-aware `assert` macro (stage 107).

### Core program shape
- Translation unit with one or more external declarations.
- Function definitions and forward declarations with return-type and
  parameter-type compatibility checking.
- Multiple functions per TU; recursive calls.
- Variadic functions (declarations, calls, and definitions with `<stdarg.h>`).
- **Struct/union arguments and return values passed by value** per the SysV AMD64 ABI.
- `main` entry point; SysV AMD64 calling convention.
- Calls into libc emitted via NASM `extern`, resolved by `gcc -no-pie`.
- `extern`-declared objects; non-static file-scope definitions emit `global`.
- **Multiple source files per invocation** (stage 96).

### Statements
- `return <expr>;` and bare `return;`; implicit void return (stage 107 checklist close).
- `if`/`else`, `while`, `do … while`, `for` including **C99 `for`-loop initializer
  declarations** scoped to the loop.
- `switch`/`case`/`default` (linear dispatch; nested switches OK).
  **`case` labels accept full compile-time integer constant expressions** including
  relational, equality, logical, and ternary operators (stage 77; extended through 104).
- `break`, `continue`, `goto` + labeled statement.
- Compound block statements with lexical scoping and shadowing.
- Expression statements.

### Declarations & types

- Integer base types `char`/`short`/`int`/`long`, all unsigned variants,
  `long long`/`unsigned long long`, `signed` keyword forms, trailing-`int` forms.
- **`float` (size=4, align=4) and `double` (size=8, align=8) scalar types** (stage 109/110):
  local declarations with FP literal initializers; global declarations (`.data` DD/DQ,
  `.bss` resd/resq); struct/union float/double members; assignment; implicit
  float→double widening via `cvtss2sd`. **FP arithmetic, unary minus, and casts
  between all scalar types now implemented (stage 110).** FP comparisons and
  function parameters deferred to stages 111/112.
- `_Bool` with value-normalization and integer promotion.
- Integer literal forms: decimal and `0x`/`0X` hexadecimal, with suffixes.
- **Floating-point literal forms** (stage 109): decimal digits with optional fraction,
  exponent (`e`/`E`), and `f`/`F` suffix; leading-dot form (`.5f`).
- `void` and generic `void *`.
- `const` qualifier at all positions; `volatile` (parse-and-track, no codegen effect);
  `restrict` (parse-and-ignore, stage 106); `inline` (parse-and-ignore, stage 107).
- Pointer types of arbitrary depth; multidimensional arrays (up to 8 dimensions).
- Function pointer types; indirect calls.
- Comma-separated init-declarator lists; brace and string initializers (local and file
  scope); **designated initializers** (`.member = value` and `[index] = value`) at
  local and global scope (stage 97).
- **Compound literals** `(type-name){ initializer-list }` (stage 98).
- File-scope global variable declarations (`.bss`/`.data`, RIP-relative).
- Storage-class specifiers `extern` and `static` at file scope, **plus block-scope
  `static` local variables** including scalars, arrays, and structs/unions (stage 71/101/102).
- `typedef` aliases; block-scope shadowing.
- Struct definitions, member access, brace initializers, whole-struct assignment/copy,
  pointer-to-struct mutation, nested structs, arrays of structs, forward declarations.
- **Named unions**; **anonymous struct/union type declarations**.
- **Enum declarations** with full integer constant expression values and forward-declared
  enum tags (stage 99).
- `NULL`; null-constant `0` accepted as a pointer argument.

### Integer constant expressions (stage 77; extended through stage 104)

A unified evaluator used for `case` labels, enumerator values, array designator
indices, file-scope integer initializers, and block-scope static scalar initializers.
Supports the full C99 operator set: `| ^ & + - << >> * / %`, unary `+ - ~ !`,
relational `< <= > >=`, equality `== !=`, logical `&& ||`, ternary `?:`,
parenthesized sub-expressions, integer/character literals, enum constants, and
`sizeof(type-name)`.

### Floating-point types and arithmetic (stages 109–110)

A per-translation-unit `.rodata` pool of `Lfc<N>` labels holds FP literal values.
Each unique raw literal text (including `f`/`F` suffix) is assigned one label;
the codegen strips the suffix before emitting `dd` (float) or `dq` (double) with
the decimal value for NASM to encode as IEEE 754. FP values travel in `xmm0`;
loads use `movss`/`movsd`.

**Stage 110** adds FP computation:
- Binary arithmetic: `addss`/`addsd`, `subss`/`subsd`, `mulss`/`mulsd`,
  `divss`/`divsd`; save/restore convention via stack (`sub rsp, 8` /
  `movss/movsd [rsp]` / `add rsp, 8`) since SSE2 has no `push xmm`.
- Unary minus: sign-bit XOR via `xorps`/`xorpd` with 16-byte-aligned `.rodata`
  masks (`Lfp_smask_f32`, `Lfp_smask_f64`), emitted on demand.
- UAC: `fp_common_arith_kind()` helper; double wins over float wins over int.
- Conversions: `cvtsi2ss`/`cvtsi2sd` (int→FP), `cvtss2sd`/`cvtsd2ss` (FP↔FP
  widening/narrowing), `cvttss2si`/`cvttsd2si` (FP→int, truncate toward zero).

### Designated initializers (C99 §6.7.8) — stage 97
- `.member = value` and `[index] = value` at local and global scope.
- Mixing designated and non-designated elements.
- Chained designators and context mismatches diagnosed.

### Compound literals (C99 §6.5.2.5) — stage 98
- Struct/union and array compound literals on the stack.
- Scalar compound literals; postfix chaining; address-of.

### Struct/union by value (System V AMD64 ABI) — stage 91-01
- Register-class (≤16 bytes) and memory-class (>16 bytes) aggregates.
- Whole-struct assignment and decl-init from struct rvalues.

### Variadic functions (`<stdarg.h>`)
- `va_start`/`va_end`/`va_arg` for GP-class types.
- `va_copy` (24-byte SysV AMD64 `va_list` struct copy — stage 107).

### Expressions
- Integer (decimal/hex), string, character, and **floating-point** literals.
- All eleven assignment operators on any modifiable lvalue.
- Arithmetic, bitwise, shift, equality/relational, and logical operators.
- **FP binary arithmetic (`+`/`-`/`*`/`/`) and unary minus** (stage 110).
- Pointer arithmetic including difference.
- Casts; integer promotions and UAC; **FP↔int and FP↔FP casts** (stage 110).
- `sizeof(type)` and `sizeof expr`.
- Address-of on any addressable lvalue; dereference; subscript.
- Compound literals; conditional (ternary) operator; comma operator.
- Function calls with any number of arguments.

### Code generation
- Single-pass walk from AST to NASM Intel-syntax assembly.
- Integer promotions, UAC, and LP64-aware conversions at every binary op.
- **FP arithmetic via SSE2** (`addss`/`addsd`, etc.); **FP unary minus via
  `xorps`/`xorpd`**; **all scalar casts** (`cvtsi2ss`/`cvtsi2sd`/`cvtss2sd`/
  `cvtsd2ss`/`cvttss2si`/`cvttsd2si`) — **stage 110**.
- FP load/store via `movss`/`movsd`; implicit float→double widening via `cvtss2sd`;
  FP literal `.rodata` pool (stage 109); sign-mask constants on demand (stage 110).
- `movzx`/`movsx` sized loads; `div`/`idiv`; `shr`/`sar`.
- String literals in `.rodata`; struct/union member addressing and block copies.
- `.data`/`.bss` globals; RIP-relative addressing.
- Block-scope and file-scope statics (scalars, arrays, structs/unions).
- Struct/union by-value marshalling; designated and compound literal init.
- Per-file codegen teardown; dynamic Vec/StrBuf storage (no fixed caps).

### Tooling & diagnostics
- `--version`, `--print-ast`, `--print-tokens`, `--print-asm` modes.
- `--max-errors=N` multiple-error collection with parser recovery.
- Multi-mode build workflow (`build.sh`): `--mode=normal`/`bootstrap`/`fallback`/`self-host`.
- Strict ISO C99 source; CMake build with `-Wall -Wextra -pedantic`.

## Stage-by-Stage Timeline (76–110)

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
| 100        | File-scope integer constant expressions; `sizeof(type-name)` in evaluator |
| 101        | Block-scope static arrays and structs/unions                  |
| 102        | Designated static aggregate initializers; multidim `.bss` fix |
| 103        | Block-scope static scalar constant-expression initializers    |
| 104        | Complete evaluator: relational, equality, logical, ternary; precedence fix |
| 106        | `restrict` keyword; comprehensive `ctype.h`/`string.h`/`stdlib.h`/`stdio.h` stubs |
| 107        | `inline` keyword; `<assert.h>`; functional `va_copy` codegen  |
| 108        | `#elifdef`/`#elifndef` preprocessor directives (C23 extension) |
| 109        | `float`/`double` types, FP literals, and stack variables      |
| **110**    | **FP arithmetic, unary minus, and scalar casts (current)**    |

## Recently Shipped (Stage 110)

**Stage 110 — float/double arithmetic, conversions, and casts.**

No new tokens, AST nodes, or parser rules. All changes are in `src/codegen.c`
and `include/codegen.h`.

**`expr_result_type` fix**: the type-inference function was using size-based
fallback (`type_kind_from_size`) for FP local and global variable references,
returning `TYPE_INT` for 4-byte floats and `TYPE_LONG` for 8-byte doubles.
Fixed by adding `type_is_fp` guards for local and global `AST_VAR_REF` paths,
and adding `AST_FLOAT_LITERAL` handling and FP-first UAC checks in
`AST_BINARY_OP` and `AST_UNARY_OP`. Without this fix the FP arithmetic codegen
path was never entered; integer instructions were emitted for FP operands.

**Binary arithmetic**: SSE2 instructions `addss`/`addsd`, `subss`/`subsd`,
`mulss`/`mulsd`, `divss`/`divsd`. Save/restore convention (SSE2 has no
`push xmm`): left operand stored to the stack with `sub rsp, 8` /
`movss/movsd [rsp], xmm0`, right operand evaluated into `xmm0`, left
restored into `xmm1` via `movss/movsd xmm1, [rsp]` / `add rsp, 8`.
Inline UAC conversions are emitted at the operand sites.

**Unary minus**: sign-bit XOR via `xorps xmm0, [rel Lfp_smask_f32]` or
`xorpd xmm0, [rel Lfp_smask_f64]`. Sign-mask constants are emitted on demand
to `.rodata` with `align 16` (SSE2 `xorps`/`xorpd` require 16-byte-aligned
128-bit operands; the initial alignment bug caused a SIGSEGV fixed during
implementation). Two new `CodeGen` flags: `fp_sign_mask_f32_emitted` /
`fp_sign_mask_f64_emitted`.

**Conversions and casts**: `emit_convert` extended for `int → float/double`
(`cvtsi2ss`/`cvtsi2sd`), `float/double → int` (`cvttss2si`/`cvttsd2si`), and
`float ↔ double` (`cvtss2sd`/`cvtsd2ss`). `emit_fp_widen_if_needed` extended
for `int → float/double`.

**UAC**: new `fp_common_arith_kind(a, b)` helper — `double` wins over `float`
wins over integer.

Eight new tests; all 1635 tests pass. Self-host C0→C1→C2 passes cleanly.

## Out of Scope (Not Yet Implemented)

- **Float/double comparisons** (Stage 111).
- **Float/double function parameters and return values** (Stage 112).
- `va_arg` for floating-point and struct-by-value types.
- Multi-character and wide-character literals.
- Compound literals at file scope.
- Bit-fields, flexible array members.
- `volatile` code-generation semantics (currently parsed and tracked only).
- Chained designators (`.a.b`, `.arr[0]`); designated union init for
  non-first members.
- Functions returning function pointers; pointer-to-array declarators
  (`(*p)[10]`); old-style (K&R) function definitions; `__attribute__` specifiers.
- GNU variadic macro extensions (`__VA_OPT__`, named variadic args,
  comma deletion).
- Object-file (`.o`) emission and separate linking; header-only precompilation.

## Architecture

```
src/
├── preprocessor.c       Two-phase preprocessor (splicing, comments, directives, macros)
├── lexer.c              Tokenizer (FP literals; line/col positions; hex/octal escapes)
├── parser.c             Recursive-descent parser; setjmp/longjmp recovery
├── ast.c                AST node lifecycle helpers (dynamic children; ast_clone)
├── ast_pretty_printer.c --print-ast renderer
├── type.c               Type system (singletons incl. float/double; const/volatile copies)
├── codegen.c            Single-pass walker → NASM asm; SysV ABI; FP literal pool; FP arith
├── compiler.c           Driver (multi-file loop; compile_one_file; per-file teardown)
├── version.c            Build/version identifier
├── vec.c                Generic growable-array container
├── strbuf.c             Dynamic character/string buffer
└── util.c               Misc helpers; compile_error_at; reset_error_state
```

The grammar is documented in `docs/grammar.md`, the parser call hierarchy
in `docs/other/stage-110-parse-tree.md`, and the feature checklist in
`docs/outlines/checklist.md`. The bootstrap workflow is driven by `build.sh`;
self-compilation findings are in `docs/self-compilation-report.md`.

## Process

Each stage produces, in order:
1. **Spec** in `docs/stages/`.
2. **Kickoff** in `docs/sessions/` — summary, change list, and spec-issue callouts.
3. **Implementation** committed in a single stage-scoped commit.
4. **Milestone summary** in `docs/milestones/`.
5. **Transcript** in `docs/sessions/` following `transcript-format.md`.

Tests live in `test/valid`, `test/invalid`, `test/print_ast`, `test/print_tokens`,
`test/print_asm`, and `test/integration/`, each driven by a `run_tests.sh` script.
