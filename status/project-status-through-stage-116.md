# ClaudeC99 Project Status — Through Stage 116

_As of 2026-06-14 (commit `9881fb0`)_

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
placeholders that return `type_int()`.

**Stages 100–108** cover tooling improvements, additional stub headers, and
preprocessor enhancements. Stage 108 adds `#elifdef` / `#elifndef`.

**Stage 109** adds **`float` and `double` scalar types**: `TYPE_FLOAT`
(4-byte SSE2 `xmm0`) and `TYPE_DOUBLE` (8-byte SSE2 `xmm0`); `movss`/`movsd`
load/store; float/double literals with `f`/`F` suffix detection and a
deduplicated `.rodata` pool; `sizeof(float)` = 4, `sizeof(double)` = 8;
implicit integer → float/double promotions; explicit casts in both directions.

**Stage 110** adds **FP arithmetic**: `addss`/`addsd`, `subss`/`subsd`,
`mulss`/`mulsd`, `divss`/`divsd`; FP unary negation via 16-byte-aligned
sign-mask XOR; Usual Arithmetic Conversions for FP; implicit and explicit
conversions (`cvtsi2ss`, `cvtsi2sd`, `cvtss2sd`, `cvtsd2ss`,
`cvttss2si`, `cvttsd2si`).

**Stage 111** adds **FP comparison operators** via `ucomiss`/`ucomisd` with
NaN-correct sequences, FP boolean context (`emit_fp_bool_to_rax`), and mixed
FP/int comparisons.

**Stage 112** completes the **floating-point calling convention**: FP arguments
pass in `xmm0`–`xmm7` (counted independently from GP args in `rdi`–`r9`);
non-variadic prologues move `xmmN` → local stack slot (`movss`/`movsd`); FP
return values land in `xmm0`; variadic prologues save `xmm0`–`xmm7` to a
176-byte register-save area (48 GP + 128 XMM, 16-byte aligned); `al` is set
to the XMM argument count before variadic calls; `va_arg(ap, double)` reads
`fp_offset` and loads from the XMM save area or overflow-arg-area; `va_arg(ap,
float)` is rejected per C99 §6.5.2.2p6. The test runner gains `.libs`
companion-file support for extra link flags (e.g., `-lm`). `test/include/math.h`
adds declarations for the full set of double and float math functions.

**Stage 113** reorganizes **1,426 test files** from flat directories into
category subfolders. Five runner scripts updated to use `find`-based recursive
discovery. No compiler source changes; all 1,650 tests pass.

**Stage 114** fixes seven compiler bugs uncovered by a legacy test suite import:
FP array subscript read/write, `expr_result_type` for FP array subscripts,
nested brace init for local and global multidimensional arrays, mixed FP/int
ternary branches, and string literal subscript (`"abc"[2]`). Also migrates 219
legacy tests to category subfolders and fixes 4 test files. All 1,841 tests pass.

**Stage 115** extends array dimension parsing to accept full C99 integer constant
expressions — arithmetic, bitwise, shift, relational, equality, logical AND/OR,
ternary, `sizeof`, parentheses, enum constants, and macro identifiers — instead
of bare integer literals. Four sites in `src/parser.c` now call `eval_const_expr()`.
No AST or codegen changes. All 1,850 tests pass.

**Stage 116** fixes two codegen bugs for global (file-scope) struct arrays: (1)
uninitialized single-dimension struct/union arrays emitted `resd N` (N × 4 bytes)
in BSS instead of `resb (N × struct_size)`, fixed by using `resb full_type->size`
in `codegen_emit_bss()` and `codegen_emit_local_statics()`; (2) string literals
initializing `char[N]` sub-arrays or struct fields emitted an 8-byte pointer
instead of inline bytes, fixed by adding `emit_string_as_bytes()` helper and
wiring it into three emission sites. All 1,857 tests pass.

**Stages completed: 237** (stage-01 through stage-116, including substages).

## Build & Test

| Component                     | Count          |
|-------------------------------|----------------|
| Source files (`src/*.c`)      | 12             |
| Header files (`include/`)     | 13             |
| Total LOC (src + include)     | 16,535         |
| Valid runtime tests           | 1,175          |
| Invalid (compile-error) tests | 258            |
| Integration tests             | 88             |
| Print-AST golden tests        | 50             |
| Print-tokens golden tests     | 100            |
| Print-asm golden tests        | 21             |
| Unit tests                    | 165            |
| **Total tests**               | **1,857**      |
| Git commits                   | 916            |

All 1,857 tests pass with no regressions — under the gcc-built compiler (C0)
and under the self-compiled C1/C2 bootstrap binaries alike.

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
- **`math.h`** — stage 112 — double and float math function declarations:
  `sin`, `cos`, `sqrt`, `pow`, `fabs`, `floor`, `ceil`, `round`, `trunc`,
  `exp`, `log`, `log2`, `log10`, `atan2`, `fmod`, `fmin`, `fmax`, `hypot`,
  `cbrt`, `tanh`, `sinh`, `cosh`, `asin`, `acos`, `atan`, and single-precision
  `*f` variants; constants `M_PI`, `M_E`, `M_SQRT2`, `M_LN2`, `M_LOG2E`.
  Programs using libm functions link with `-lm` via `.libs` companion files.

### Core program shape
- Translation unit with one or more external declarations.
- Function definitions and forward declarations (return-type and
  parameter-type compatibility checking across redeclarations).
- Multiple functions per translation unit; recursive calls.
- Variadic function **declarations, external calls, and definitions**;
  callee-side `<stdarg.h>` access (see Variadic functions below).
- **Struct/union arguments and return values passed by value** per the
  System V AMD64 ABI (see Struct/union by value below).
- **Float/double function arguments and return values** per the
  System V AMD64 ABI (see Floating-point below).
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
  with unary `~ !` and parenthesized sub-expressions (stage 77; extended stage 99).
- `break`, `continue`, `goto` + labeled statement.
- Compound block statements with lexical scoping and shadowing.
- Expression statements.

### Declarations & types
- Integer base types `char`/`short`/`int`/`long`, all unsigned variants,
  `long long` / `unsigned long long`, the `signed` keyword forms, and
  trailing-`int` forms.
- **`float` and `double` scalar types** (stage 109) — 4-byte/8-byte SSE2
  values; `sizeof(float)` = 4, `sizeof(double)` = 8; float/double literals
  with `f`/`F` suffix detection and deduplicated `.rodata` constant pool.
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
  RIP-relative addressing).
- Storage-class specifiers `extern` and `static` at file scope, **plus
  block-scope `static` local variables** that persist across calls and
  are emitted to `.bss` (uninitialized) or `.data` (initialized) with
  RIP-relative `[rel Lstatic_func_N]` addressing.
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
  (arithmetic, shift, bitwise, unary `~ !`, parentheses, and references
  to previously-defined enum constants) — stage 99.
  Forward-declared enum tags (`typedef enum Status Status;` before the
  body) are supported — stage 99.
- **Constant-expression array dimension bounds** (stage 115): array dimensions
  in declarations and type names accept full C99 integer constant expressions
  (arithmetic, bitwise, shift, sizeof, enum constants, macro identifiers),
  not just bare integer literals.

### Integer constant expressions (stage 77; extended stage 99)

A unified nine-level evaluator (`eval_const_expr`) is used for:
- **`case` labels**: `case 1 << 2:` (evaluates to 4), `case PERM_READ | PERM_WRITE:`.
- **Enumerator values**: `FLAG_READ = 1 << 0`, `STEP = BASE + 5`, `ALL = ~0`,
  `SMALL = (4 * 8)`.
- **Array designator indices**: `[2 + 1] = value`.

Operators supported (loosest to tightest): `| ^ & + - << >> * / %`
(division-by-zero → compile error); unary `+ - ~ !`; parenthesized
sub-expressions; integer/character literals; enum constants by name.

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
- Uninitialized global and block-scope static struct/union arrays now
  correctly reserve `resb (N × sizeof(struct))` bytes in BSS. Global `char[N]`
  sub-arrays and struct `char[N]` fields initialized from string literals are
  emitted as inline bytes — stage 116.

### Floating-point types and calling convention (stages 109–112)

- **`float` and `double` scalar values** stored in SSE2 XMM registers
  (`movss`/`movsd` load/store).
- **FP arithmetic**: `+`, `-`, `*`, `/` for both float and double
  (`addss`/`subss`/`mulss`/`divss`, `addsd`/`subsd`/`mulsd`/`divsd`).
- **FP unary negation** via 16-byte-aligned sign-mask XOR.
- **Usual Arithmetic Conversions for FP**: float + double → double;
  int + float → float; int + double → double.
- **Implicit and explicit FP conversions**: integer → float/double
  (`cvtsi2ss`/`cvtsi2sd`); float ↔ double (`cvtss2sd`/`cvtsd2ss`);
  float/double → integer with truncation (`cvttss2si`/`cvttsd2si`).
- **FP comparison operators** via `ucomiss`/`ucomisd` with NaN-correct
  branch sequences (stage 111); FP boolean context via `emit_fp_bool_to_rax`;
  mixed FP/int comparisons.
- **SysV AMD64 FP calling convention** (stage 112): FP arguments in
  `xmm0`–`xmm7` (counted independently from GP registers `rdi`–`r9`);
  non-variadic prologues copy `xmmN` to a local stack slot; FP return
  values in `xmm0`.
- **Variadic FP arguments** (stage 112): prologues save `xmm0`–`xmm7` to
  a 176-byte register-save area (48 GP + 128 XMM bytes, 16-byte aligned at
  `[rbp-128]`); `al` set to XMM count before variadic calls; `va_arg(ap,
  double)` reads `fp_offset` and loads the XMM value from the save area or
  the overflow-arg-area.

### Variadic functions (`<stdarg.h>`)
- `va_start` / `va_end` / `va_arg` for GP-class types (int, long, pointers)
  and **`double`** (stage 112).
- `va_copy` is recognized but its codegen is still a no-op stub.
- `va_arg(ap, float)` is rejected per C99 §6.5.2.2p6 (float promotes
  to double in variadic calls).

### Expressions
- Integer (decimal/hex), string, and character literals; adjacent
  string-literal concatenation; hex/octal escapes; variable references.
- **Float and double literals** with `f`/`F` suffix detection.
- **String literal subscripting**: `"abc"[2]` accepted as a postfix subscript
  expression — stage 114.
- All eleven assignment operators on any modifiable lvalue.
- Arithmetic, bitwise, shift, equality/relational, and logical operators.
- **FP arithmetic and comparisons** (stages 110–111).
- Pointer arithmetic including difference (`long`).
- Casts; integer promotions and UAC; **FP UAC and conversions** (stages 109–110).
- `sizeof(type)` and `sizeof expr` (operand not evaluated).
- Address-of on any addressable lvalue; dereference; subscript.
- Compound literals; conditional (ternary) operator; comma operator.
- Function calls with any number of arguments.

### Code generation
- Single-pass walk from AST to NASM Intel-syntax assembly.
- Integer promotions, UAC, and LP64-aware conversions at every binary op.
- **SSE2 FP operations**: `movss`/`movsd`, `addss`/`addsd` etc., `ucomiss`/`ucomisd`.
- **SysV FP calling convention**: `compute_call_layout` classifies args as
  GP or XMM; XMM args emitted to `xmm0`–`xmm7`; FP prologues bind XMM
  params to local slots.
- `movzx`/`movsx` sized loads; `div`/`idiv`; `shr`/`sar`.
- String literals in `.rodata`; FP constants in deduplicated `.rodata` pool.
- Struct/union member addressing and block struct copies (`rep movsb`);
  `.data`/`.bss` globals.
- Struct/union by-value marshalling; designated and compound literal init.
- Per-file codegen teardown; dynamic Vec/StrBuf storage (no fixed caps).

### Tooling & diagnostics
- `--version`, `--print-ast`, `--print-tokens`, `--print-asm` modes.
- `--max-errors=N` multiple-error collection with parser recovery.
- Multi-mode build workflow (`build.sh`): `--mode=normal` / `bootstrap` /
  `fallback` / `self-host` (C0→C1→C2 with test runs and checkpoint commits).
- **`.libs` companion files** (stage 112): `test/valid/run_tests.sh` reads
  `<name>.libs` to pass extra linker flags (e.g., `-lm`) when linking
  individual test objects.
- Strict ISO C99 source; CMake build with `-Wall -Wextra -pedantic`.

## Stage-by-Stage Timeline (76–116)

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
| 99         | `typedef enum` completion — const expr evaluator + forward refs|
| 100–108    | Tooling, stub headers, `#elifdef`/`#elifndef` (stage 108)     |
| 109        | `float` and `double` scalar types; SSE2 load/store; literals  |
| 110        | FP arithmetic; unary negation; UAC; int/FP conversions        |
| 111        | FP comparison operators; FP boolean context; mixed FP/int     |
| 112        | FP calling convention (xmm0–xmm7); va_arg double; math.h      |
| 113        | Test suite reorganization: category subfolders, find-based discovery  |
| 114        | Legacy test fixes: FP subscript, nested brace init, string subscript  |
| 115        | Constant expressions in array dimension bounds                |
| **116**    | **Global struct array BSS fix; char[N] string-literal init (current)**|

## Recently Shipped (Stages 113–116)

**Stage 113 — Test Suite Reorganization.** 1,426 test files reorganized from
flat directories into category subfolders under `test/valid/` (21 categories)
and `test/invalid/` (9 categories + legacy). Five runner scripts updated to
use `find`-based recursive discovery so new tests placed in any subfolder are
picked up automatically. No compiler source changes; all 1,650 tests pass.

**Stage 114 — Legacy Test Fixes.** Seven compiler bugs fixed: (1) FP array
subscript write emits `movss`/`movsd` instead of `mov`; (2) FP array subscript
read emits `movss xmm0, [rax]`/`movsd xmm0, [rax]` instead of truncating via
`movsxd`; (3) `expr_result_type` for `AST_ARRAY_INDEX` returns FP element kind
directly; (4) nested brace local array init adds `emit_local_array_init()`
recursive helper; (5) nested brace global array init adds
`emit_global_array_elements()` recursive helper; (6) mixed FP/int ternary
widens branches to a common FP type; (7) string literal subscript
(`"abc"[2]`) accepted in both parser and codegen. 219 legacy tests migrated to
13 category subfolders. All 1,841 tests pass.

**Stage 115 — Constant Expressions in Array Dimension Bounds.** Four sites in
`parse_declarator` and `parse_type_name` now call `eval_const_expr()` for array
bounds, enabling `int a[N*2]`, `int a[sizeof(int)*8]`, `int a[MACRO]`, and
multidimensional forms. The evaluator (available since stage 99) is unchanged;
only the parser call sites were updated. All 1,850 tests pass.

**Stage 116 — Global Struct Array BSS Fix and char[N] String Init.** Two
codegen bugs corrected: (1) `codegen_emit_bss()` and `codegen_emit_local_statics()`
now emit `resb full_type->size` for single-dimension arrays instead of
`bss_res_directive(element_kind) × N` — fixing underallocation for struct/union
arrays; (2) new `emit_string_as_bytes()` helper wired into three emission sites
(`codegen_emit_data()`, `emit_global_array_elements()`, `emit_global_struct()`)
so `char[N]` sub-arrays and struct fields initialized from string literals emit
inline bytes rather than 8-byte pointers. Seven new valid tests; all 1,857 tests pass.

## Out of Scope (Not Yet Implemented)

- `long double` (x87 80-bit; explicitly out of scope).
- `va_arg` for struct-by-value types; `va_arg(ap, float)` (rejected per C99);
  `va_copy` codegen (still a no-op stub).
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
- `#pragma` (including `#pragma once`).
- GNU variadic macro extensions (`__VA_OPT__`, named variadic args,
  comma deletion).
- Object-file (`.o`) emission and separate linking; header-only precompilation.
- General integer constant expressions in object initializers at file scope
  (`int x = 1 + 2;` — the evaluator is not wired into the global initializer path).
- Comparison and logical operators (`==`, `!=`, `&&`, `||`) in
  `eval_const_expr` (not needed for enumerator values or `case` labels
  in practice).

## Architecture

```
src/
├── preprocessor.c       Two-phase preprocessor (splicing, comments, directives, macros)
├── lexer.c              Tokenizer (line/col positions; hex literals; hex/octal escapes; FP literals)
├── parser.c             Recursive-descent parser, builds AST; setjmp/longjmp recovery
├── ast.c                AST node lifecycle helpers (dynamic children array; ast_clone)
├── ast_pretty_printer.c --print-ast renderer
├── type.c               Type system (singletons + heap pointer chains; float/double; const/volatile)
├── codegen.c            Single-pass walker → NASM Intel-syntax asm; SysV ABI; SSE2 FP
├── compiler.c           Driver (multi-file loop; compile_one_file; per-file teardown)
├── version.c            Build/version identifier (VERSION_BUILTBY)
├── vec.c                Generic growable-array container (stage 95-02)
├── strbuf.c             Dynamic character/string buffer (stage 95-03)
└── util.c               Misc helpers; compile_error_at / compile_warning_at; reset_error_state
```

The grammar is documented in `docs/grammar.md`, the parser call hierarchy
in `docs/other/stage-116-parse-tree.md`, and the feature checklist in
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
