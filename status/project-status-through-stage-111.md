# ClaudeC99 Project Status — Through Stage 111

_As of 2026-06-13 (commit `a337b3a`)_

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
workflow (`build.sh`); stage 94 adds a repeatable `--mode=self-host`
C0→C1→C2 cycle. Stages 95-02 through 95-12 are an internal
**dynamic-storage refactor**: new `Vec` and `StrBuf` containers replace the
parser/codegen fixed-capacity tables, and all token/AST/parser/codegen name
and label text moves from `char[MAX_NAME_LEN]` buffers into `const char *`
pointers backed by a lexer-owned string pool. **Stage 96** adds **multi-file
compilation**. **Stage 97** adds **designated initializers** (C99 §6.7.8).
**Stage 98** adds **compound literals** (C99 §6.5.2.5). **Stage 99**
completes **`typedef enum`** with a full integer constant expression
evaluator and forward-declared enum tags.

**Stage 100** extends the constant expression evaluator to support `sizeof`
and wires it into **file-scope integer initializers** — `int X = sizeof(int)
* 1024;` now compiles at file scope.

**Stage 101** adds **block-scope static arrays and struct/union variables**:
static local aggregates are emitted to `.bss`/`.data` with RIP-relative
addressing, matching the existing scalar-static path.

**Stage 102** extends static aggregate support with **designated initializers
for block-scope static aggregates**, fixes 2D array member-init ordering, and
corrects a BSS layout bug for multi-field structs.

**Stage 103** moves the constant-expression initializer evaluator
(`eval_const_init`) into codegen and fixes a hex-literal parsing bug and a
missing `sizeof` fallback in that path.

**Stage 104** completes the integer constant expression evaluator with
**relational, equality, logical-and/or, and ternary operators**, plus a
pre-existing precedence bug fix (swapped additive/shift call order). The same
five operators are mirrored in `eval_const_init` for block-scope static
scalar initializers.

**Stage 106** adds **`restrict` as a parse-and-ignore keyword** at all
pointer qualifier positions, completes a round of major **stub header
additions** (`strtoul`, `strtod`, `atoi`, `qsort`, `getenv`, type-generic
math macros, …), and fixes a **long-long function-return codegen bug**.

**Stage 107** adds **`inline` as a parse-and-ignore keyword**, provides
**functional `va_copy` codegen** (three 8-byte copy instructions emitted for
`__builtin_va_copy`), adds an **`assert.h` stub**, and fixes a preprocessor
**`__FILE__`/`__LINE__` expansion bug** inside nested `#include` chains.

**Stage 108** adds the **`#elifdef` and `#elifndef` preprocessor directives**.

**Stage 109** adds **`float` and `double` as first-class scalar types**
throughout the pipeline: new lexer tokens, `AST_FLOAT_LITERAL`, `TYPE_FLOAT`
/ `TYPE_DOUBLE`, stack-slot allocation, SSE2 load/store (`movss`/`movsd`),
and a `.rodata` floating-point literal pool (`Lfc<N>` labels).

**Stage 110** adds **floating-point arithmetic, implicit/explicit conversions,
and casts**: `addss`/`addsd`, `subss`/`subsd`, `mulss`/`mulsd`,
`divss`/`divsd`; unary negation via sign-mask XOR; Usual Arithmetic
Conversions for FP (`fp_common_arith_kind`); `cvtsi2ss`/`cvtsi2sd` for
int→FP, `cvtss2sd`/`cvtsd2ss` between FP widths, and `cvttss2si`/
`cvttsd2si` for truncating FP→int casts.

**Stage 111** adds **floating-point comparison operators and boolean
contexts**: `ucomiss`/`ucomisd` comparison instruction, NaN-correct `==`
(`sete+setnp+and`) and `!=` (`setne+setp+or`) sequences, ordered relational
operators (`<`/`<=`/`>`/`>=` via `setb`/`setbe`/`seta`/`setae`), FP boolean
context in `if`/`while`/`for`/ternary via `emit_fp_bool_to_rax`, and logical
`!` on FP operands (NaN-correct: `sete+setnp+and`). Mixed FP/int comparisons
promote the integer side with `cvtsi2ss`/`cvtsi2sd`.

**Stages completed: 229** (stage-01 through stage-111, including substages).

## Build & Test

| Component                     | Count          |
|-------------------------------|----------------|
| Source files (`src/*.c`)      | 12             |
| Header files (`include/`)     | 13             |
| Total LOC (src + include)     | 16,090         |
| Valid runtime tests           | 958            |
| Invalid (compile-error) tests | 255            |
| Integration tests             | 86             |
| Print-AST golden tests        | 50             |
| Print-tokens golden tests     | 100            |
| Print-asm golden tests        | 21             |
| **Total tests**               | **1,643**      |
| Git commits                   | 884            |

All 1,643 tests pass with no regressions — under the gcc-built compiler and
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
- **`#elifdef` / `#elifndef`** — stage 108.
- **Preprocessor conditional expressions**: full evaluator for
  `#if` / `#elif` supporting integer literals, `defined(NAME)`,
  object-like macro expansion, undefined-identifier-as-0, unary
  `!` / `-` / `+` / `~`, parentheses, `==` / `!=` / `<` / `<=` /
  `>` / `>=`, `&&` / `||`, `+` / `-` / `*` / `/` / `%`, `&` / `^` /
  `|`, `<<` / `>>`.
- **`#error message`**.
- **Predefined macros — standard**: `__FILE__`, `__LINE__`, `__DATE__`,
  `__TIME__`, `__STDC__`, `__STDC_VERSION__`, `__STDC_HOSTED__`,
  `__CLAUDEC99__`. Stage 107 fixes `__FILE__`/`__LINE__` expansion
  inside nested `#include` chains.
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
  `strtol`, `strtoul`, `strtod`, `atoi`, `qsort`, `getenv`, `abs`.
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
- **`assert.h`** — `assert()` macro expanding to a call-chain-based
  diagnostic and abort on failure — stage 107.

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
  the loop. **Float/double conditions** are handled via SSE2 comparisons
  against zero using `emit_fp_bool_to_rax` — stage 111.
- `switch` / `case` / `default` (linear dispatch; nested switches OK).
  **`case` labels accept full compile-time integer constant expressions** —
  integer/character literals, enum constants, and all of `* / % << >> + - &
  ^ |` with unary `~ !` and parenthesized sub-expressions (stage 77;
  extended stage 99 / 104).
- `break`, `continue`, `goto` + labeled statement.
- Compound block statements with lexical scoping and shadowing.
- Expression statements.

### Declarations & types
- Integer base types `char`/`short`/`int`/`long`, all unsigned variants,
  `long long` / `unsigned long long`, the `signed` keyword forms, and
  trailing-`int` forms.
- **`float` and `double` scalar types** — stage 109. Stack slots: 4 bytes
  (4-byte aligned) for `float`, 8 bytes (8-byte aligned) for `double`.
  `sizeof(float) == 4`, `sizeof(double) == 8`. FP values live in `xmm0`
  (analogous to `rax` for integers). Literals emitted to `.rodata` as
  `DD`/`DQ` with deduplicated `Lfc<N>` labels. Allowed in struct/union
  members, local variables, globals, parameters (by type; calling-convention
  pass-through pending stage 112).
- `_Bool` with value-normalization and integer promotion.
- Integer literal forms: decimal and **`0x`/`0X` hexadecimal** literals,
  with suffixes `U`/`L`/`UL`/`LL`/`ULL` and overflow-aware typing.
- Float/double literals: decimal (`1.5`, `.5`, `1.0e-3`); `f`/`F` suffix
  for `float`, no suffix for `double` — stage 109.
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
- **`restrict` qualifier**: tokenized and parsed at all pointer-qualifier
  positions; silently discarded with no AST or semantic effect — stage 106.
- **`inline` function specifier**: tokenized and parsed in
  `parse_declaration_specifiers`; silently discarded — stage 107.
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
  temporary objects allocated on the stack — stage 98.
- File-scope (global) variable declarations (`.bss` / `.data`,
  RIP-relative addressing). **File-scope integer initializers accept
  full constant expressions** including `sizeof`, arithmetic, bitwise,
  and shift operators — stage 100.
- Storage-class specifiers `extern` and `static` at file scope, **plus
  block-scope `static` local variables** (scalar — always; **array and
  struct/union — stage 101**) that persist across calls and are emitted
  to `.bss` (uninitialized) or `.data` (initialized) with RIP-relative
  `[rel Lstatic_func_N]` addressing. **Designated initializers** for
  static aggregates are supported — stage 102.
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
  (arithmetic, shift, bitwise, relational, equality, logical-and/or,
  ternary, unary `~ !`, parentheses, and references to
  previously-defined enum constants) — stage 99 / 104.
  Forward-declared enum tags (`typedef enum Status Status;` before the
  body) are supported — stage 99.

### Integer constant expressions (stage 77; extended stages 99, 100, 104)

A unified thirteen-level evaluator (`eval_const_expr`) is used for:
- **`case` labels**: `case 1 << 2:` (evaluates to 4), `case PERM_READ | PERM_WRITE:`.
- **Enumerator values**: `FLAG_READ = 1 << 0`, `STEP = BASE + 5`, `ALL = ~0`,
  `COND = A > B ? A : B`.
- **Array designator indices**: `[2 + 1] = value`.
- **File-scope integer initializers**: `int X = sizeof(int) * 1024;` — stage 100.
- **Block-scope static scalar initializers** (`eval_const_init`): same
  operator set mirrored in `src/codegen.c` — stages 103, 104.

Operators supported (loosest to tightest): `?: || && | ^ & == != < <= > >=
<< >> + - * / %` (division-by-zero → compile error); unary `+ - ~ !`;
`sizeof(type-name)`; parenthesized sub-expressions; integer/character
literals; enum constants by name.

### Designated initializers (C99 §6.7.8)
- **`.member = value`** member designators in struct brace initializers
  at both local and global scope.
- **`[index] = value`** array index designators at both local and global
  scope; the index must be a non-negative constant integer expression.
- Mixing designated and non-designated elements is supported.
- Chained designators and context mismatches are diagnosed.
- **Static aggregate designated initializers** (block-scope `static` arrays
  and structs) — stage 102.

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
- `va_copy` emits three 8-byte copy instructions via `__builtin_va_copy`
  (functional codegen added stage 107; was a no-op stub before).

### Floating-point arithmetic and comparisons (stages 109–111)

- **FP binary arithmetic** (`+`, `-`, `*`, `/`): `addss`/`addsd`,
  `subss`/`subsd`, `mulss`/`mulsd`, `divss`/`divsd`. Left operand saved
  to stack (`sub rsp,8` / `movsd [rsp],xmm0`), right evaluated into
  `xmm0`, left restored to `xmm1`, then op emitted.
- **Unary FP negation**: sign-bit XOR via 16-byte-aligned `.rodata` masks
  (`Lfp_smask_f32` / `Lfp_smask_f64`).
- **Usual Arithmetic Conversions for FP** (C99 §6.3.1.8): double wins
  over float wins over any integer kind (`fp_common_arith_kind`).
- **Implicit conversions**: int→float (`cvtsi2ss`), int→double
  (`cvtsi2sd`), float→double (`cvtss2sd`), double→float (`cvtsd2ss`).
- **Explicit casts**: `(float)`, `(double)` (via CVT instructions);
  `(int)` from FP uses truncating `cvttss2si`/`cvttsd2si`.
- **FP comparison operators** (`<`, `<=`, `>`, `>=`, `==`, `!=`):
  `ucomiss`/`ucomisd` sets CF/ZF/PF; ordered comparisons use
  `setb`/`setbe`/`seta`/`setae`; NaN-correct `==` uses `sete+setnp+and`;
  NaN-correct `!=` uses `setne+setp+or` — stage 111.
- **FP boolean context** (`if`/`while`/`for`/ternary/`!` applied to FP
  value): `emit_fp_bool_to_rax` compares `xmm0` to zero via
  `xorps/xorpd; ucomiss/ucomisd; setne al; setp cl; or al,cl;
  movzx rax,al`. NaN is treated as true; `!NaN == 0` — stage 111.
- **Mixed FP/int comparisons**: integer operand promoted via
  `cvtsi2ss`/`cvtsi2sd` to the common FP kind before comparison.

### Expressions
- Integer (decimal/hex), string, and character literals; adjacent
  string-literal concatenation; hex/octal escapes; variable references.
- **Float/double literals** with SSE2 register results — stage 109.
- All eleven assignment operators on any modifiable lvalue.
- Arithmetic, bitwise, shift, equality/relational, and logical operators
  (integer and FP — stage 111 for FP comparisons).
- Pointer arithmetic including difference (`long`).
- Casts; integer promotions and UAC; FP promotions and FP UAC — stage 110.
- `sizeof(type)` and `sizeof expr` (operand not evaluated). `sizeof(float)`
  = 4, `sizeof(double)` = 8 — stage 109.
- Address-of on any addressable lvalue; dereference; subscript.
- Compound literals; conditional (ternary) operator (with FP condition
  support — stage 111); comma operator.
- Function calls with any number of arguments.

### Code generation
- Single-pass walk from AST to NASM Intel-syntax assembly.
- Integer promotions, UAC, and LP64-aware conversions at every binary op.
- `movzx`/`movsx` sized loads; `div`/`idiv`; `shr`/`sar`.
- **SSE2 FP load/store/arithmetic/comparison** (`movss`/`movsd`,
  `addss`/`addsd`, `ucomiss`/`ucomisd`, etc.) — stages 109–111.
- **FP literal pool** in `.rodata` with deduplicated `Lfc<N>` labels.
- **Sign masks** (`Lfp_smask_f32`/`Lfp_smask_f64`): 16-byte-aligned,
  emitted on demand.
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

## Stage-by-Stage Timeline (76–111)

Stages 01–65 are catalogued in `project-status-through-stage-65.md`, and
66–75 in `project-status-through-stage-75-06.md`; new stages since:

| Stage      | Focus                                                                   |
|------------|-------------------------------------------------------------------------|
| 76         | `for`-loop initializer declarations (C99)                               |
| 77         | Enum / constant-expression `case` labels                                |
| 78         | General postfix expression chaining                                     |
| 79         | General-lvalue compound assignment                                      |
| 80         | Prefix/postfix `++`/`--` on general lvalues                             |
| 81         | Header updates (`putchar`, `calloc`); `!` on pointers; limits           |
| 82-01      | `const` qualifiers in struct/union members                              |
| 82-02      | const-qualified member lvalue (subscript) rules                         |
| 82-03      | `const` in type-name contexts (sizeof/cast/va_arg)                      |
| 82-04      | Minimal `volatile` handling                                             |
| 82-05      | const member / pointer-to-const-struct diagnostics                      |
| 83         | Project source converted to strict ISO C99                              |
| 84         | Standard streams `stdin`/`stdout`/`stderr`; extern objects              |
| 84-02      | `stdlib.h` `exit()` declaration                                         |
| 85         | Member-array to pointer decay; char-member string init                  |
| 85-01      | `string.h` additional declarations                                      |
| 86         | Multidimensional array support                                          |
| 87         | `stdio.h` file-position / read stubs (`fseek`/`ftell`/`fread`)         |
| 88         | Hex (`\xNN`) and octal (`\NNN`) character/string escapes                |
| 89         | Adjacent string-literal concatenation                                   |
| 90         | Hexadecimal integer literals                                            |
| 91         | Address-of member lvalues                                               |
| 91-01      | Struct/union by-value parameters and returns (SysV ABI)                 |
| 92         | Self-compilation validation — **full self-hosting achieved**            |
| 93         | Bootstrap build workflow; `VERSION_BUILTBY`                             |
| 94         | Self-host validation; `build.sh --mode=self-host` C0→C1→C2             |
| 95-01      | Fixed-capacity inventory (docs only)                                    |
| 95-02      | `Vec` generic growable-array module                                     |
| 95-03      | `StrBuf` dynamic string-buffer module                                   |
| 95-04      | Low-risk static arrays → `Vec`                                          |
| 95-05      | Medium-risk static arrays → `Vec`                                       |
| 95-06      | High-risk static arrays → `Vec` (two latent bugs fixed)                 |
| 95-07      | Remaining static usages → `Vec`; call-layout bounds guard               |
| 95-08      | Token text → pointer + length (255-byte string cap removed)             |
| 95-09      | `ASTNode.value` → `const char *`                                        |
| 95-10      | parser.h name/tag fields → `const char *`                               |
| 95-11      | codegen.h name/label fields → `const char *`                            |
| 95-12      | `#if` unary buffer → `StrBuf`; switch labels → `Vec`                    |
| 96         | Multi-file compilation; per-file teardown                               |
| 97         | Designated initializers (`.member =`, `[index] =`)                      |
| 98         | Compound literals `(T){ … }` — struct, array, scalar                    |
| 99         | `typedef enum` completion — const expr evaluator + forward refs         |
| 100        | `sizeof` in const exprs; file-scope integer constant initializers       |
| 101        | Block-scope static arrays and struct/union variables                    |
| 102        | Static aggregate designated init; 2D array init fix; BSS layout fix     |
| 103        | `eval_const_init` in codegen; hex literal and sizeof fallback fixes     |
| 104        | Const expr: relational/equality/logical/ternary; precedence bug fix     |
| 106        | `restrict` parse-and-ignore; stub header additions; long-long ret fix   |
| 107        | `inline` parse-and-ignore; `va_copy` codegen; `assert.h`; `__FILE__` fix|
| 108        | `#elifdef` / `#elifndef` preprocessor directives                        |
| 109        | `float`/`double` types, literals, load/store — SSE2 pipeline            |
| 110        | FP arithmetic, conversions, casts (`cvtsi2ss`, `addsd`, unary neg, …)  |
| **111**    | **FP comparisons and boolean contexts (`ucomiss`, NaN-correct) (current)**|

## Recently Shipped (Stage 111)

**Stage 111 — floating-point comparisons and boolean contexts.**

Float/double values can now appear in all comparison and control-flow
contexts. The implementation builds on the stage 110 save/restore
convention (left in `xmm1`, right in `xmm0` after stack pop) to
position operands for `ucomisd`/`ucomiss`.

**Comparison operators.**  After comparing via `ucomiss xmm1, xmm0` or
`ucomisd xmm1, xmm0`, the result is materialized in `al`:

| Operator | Flags used          | Set sequence                  |
|----------|---------------------|-------------------------------|
| `<`      | CF=1                | `setb al`                     |
| `<=`     | CF=1 ∨ ZF=1         | `setbe al`                    |
| `>`      | CF=0 ∧ ZF=0         | `seta al`                     |
| `>=`     | CF=0                | `setae al`                    |
| `==`     | ZF=1 ∧ PF=0         | `sete al; setnp cl; and al,cl`|
| `!=`     | ZF=0 ∨ PF=1         | `setne al; setp cl; or al,cl` |

PF=1 when either operand is NaN (`ucomiss`/`ucomisd` sets PF on
unordered comparison). The `==` and `!=` sequences are the standard
NaN-correct patterns mandated by C99 §6.5.9p4.

**Boolean context.** A new helper `emit_fp_bool_to_rax` (placed in the
FP helper section of `src/codegen.c` to avoid C99 forward-declaration
issues) emits:
```nasm
xorpd xmm1, xmm1      ; zero reference
ucomisd xmm0, xmm1    ; compare FP value against 0.0
setne al               ; non-zero OR NaN → 1
setp  cl               ; unordered (NaN) → 1
or    al, cl
movzx rax, al
```
This helper is called by: `emit_cond_cmp_zero` (for `if`/`while`/`for`
conditions), `AST_CONDITIONAL_EXPR` (ternary), and the `!` operator on FP
operands. NaN is treated as truthy (non-zero), matching common C
implementations; `!NaN` is 0.

**Mixed FP/int comparisons.** When one operand is FP and the other is an
integer, the integer side is promoted via `cvtsi2ss`/`cvtsi2sd` using the
same `fp_common_arith_kind` helper from stage 110.

Eight new tests were added; all 1,643 tests pass at C0, C1, and C2.

## Out of Scope (Not Yet Implemented)

- **Floating-point function parameters and return values** (stage 112):
  SysV AMD64 passes FP args in `xmm0`–`xmm7`; currently FP parameters
  are declared but not yet passed or returned through the FP register path.
- `va_arg` for floating-point types (stage 112); `float` arguments in
  variadic calls are promoted to `double` per C99 §6.5.2.2p6.
- Multi-character constants (`'ab'`); wide-character literals.
- Compound literals at file scope.
- Bit-fields, flexible array members.
- `volatile` code-generation semantics (currently parsed and tracked only).
- Chained designators (`.a.b`, `.arr[0]`); designated union init for
  non-first members.
- Functions returning function pointers; pointer-to-array declarators
  (`(*p)[10]`); old-style (K&R) function definitions; `__attribute__`
  specifiers.
- `#pragma` (including `#pragma once`).
- GNU variadic macro extensions (`__VA_OPT__`, named variadic args,
  comma deletion).
- Object-file (`.o`) emission and separate linking; header-only
  precompilation.
- `long double` (x87 80-bit); explicitly out of scope for the FP
  implementation plan.

## Architecture

```
src/
├── preprocessor.c       Two-phase preprocessor (splicing, comments, directives, macros)
├── lexer.c              Tokenizer (line/col positions; hex literals; hex/octal escapes; FP literals)
├── parser.c             Recursive-descent parser, builds AST; setjmp/longjmp recovery
├── ast.c                AST node lifecycle helpers (dynamic children array; ast_clone)
├── ast_pretty_printer.c --print-ast renderer
├── type.c               Type system (singletons + heap pointer chains; const/volatile; TYPE_FLOAT/DOUBLE)
├── codegen.c            Single-pass walker → NASM Intel-syntax asm; SysV ABI; SSE2 FP codegen
├── compiler.c           Driver (multi-file loop; compile_one_file; per-file teardown)
├── version.c            Build/version identifier (VERSION_BUILTBY)
├── vec.c                Generic growable-array container (stage 95-02)
├── strbuf.c             Dynamic character/string buffer (stage 95-03)
└── util.c               Misc helpers; compile_error_at / compile_warning_at; reset_error_state
```

The grammar is documented in `docs/grammar.md`, the parser call hierarchy
in `docs/other/stage-111-parse-tree.md`, and the feature checklist in
`docs/outlines/checklist.md` — each updated alongside any stage that
touches it. The bootstrap workflow is driven by `build.sh`; the
self-compilation findings are recorded in `docs/self-compilation-report.md`.
The floating-point implementation plan is in `docs/outlines/floating-point-plan.md`.

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
