# ClaudeC99 Project Status — Through Stage 101

_As of 2026-06-10 (commit `d03dc8f`)_

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

**Stage 100** wires **`eval_const_expr` into file-scope integer-typed
variable initializers**: `int PAGE = 1 << 12;`, `int MASK = FLAG_A | FLAG_B;`,
`int BUF = sizeof(int) * 1024;`, and `int NEG = -(3 * 4);` all now compile
at file scope. Additionally, `sizeof(type-name)` is added to the constant
expression evaluator (`eval_const_primary`), and a pre-existing bug where
`sizeof(void *)` was rejected in general expressions is fixed.

**Stage 101** lifts the **block-scope static aggregate restriction**: block-scope
`static` local variables can now hold `TYPE_ARRAY`, `TYPE_STRUCT`, and
`TYPE_UNION` types in addition to scalars and pointers. Static local arrays and
structs use RIP-relative `Lstatic_func_N` labels — exactly like file-scope
globals — and are emitted to `.bss` (uninitialized) or `.data` (initialized).
This is a codegen-only stage.

**Stages completed: 208** (stage-01 through stage-101, including substages).

## Build & Test

| Component                     | Count          |
|-------------------------------|----------------|
| Source files (`src/*.c`)      | 12             |
| Header files (`include/`)     | 13             |
| Total LOC (src + include)     | 14,837         |
| Valid runtime tests           | 880            |
| Invalid (compile-error) tests | 250            |
| Integration tests             | 86             |
| Print-AST golden tests        | 50             |
| Print-tokens golden tests     | 100            |
| Print-asm golden tests        | 21             |
| **Total tests**               | **1,552**      |
| Git commits                   | 822            |

All 1,552 tests pass with no regressions — under the gcc-built compiler and
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
- **Predefined macros — standard**: `__FILE__`, `__LINE__`, `__DATE__`,
  `__TIME__`, `__STDC__`, `__STDC_VERSION__` (`199901`),
  `__STDC_HOSTED__`, `__CLAUDEC99__`.
- **Predefined macros — ABI/target** (unconditionally injected):
  `__x86_64__`, `__linux__`, `__unix__`, `__LP64__`, `_LP64`,
  `__CHAR_BIT__`, and the `__SIZEOF_*__` family (`CHAR`, `SHORT`, `INT`,
  `LONG`, `POINTER`, `SIZE_T`).
- **Command-line defines**: `-DNAME` (defines as `1`) and
  `-DNAME=VALUE`; injected before preprocessing.
- **Include search paths**: `-I<dir>` or `-I <dir>`; quoted includes
  search the including file's directory first, then `-I` directories;
  angle-bracket includes search `-I` directories only.
- **`--sysroot=<dir>`**: rewrites all absolute-style `-I` paths.

### Statements

- `if` / `else` with optional else branch.
- `while` / `do`-`while` loops.
- `for` loops with C99 init-declarations (`for (int i = 0; …)`).
- `switch` / `case` / `default`: case labels support integer literals,
  character literals, enum constants, and compile-time constant expressions
  with all of `* / % << >> + - & ^ |` with unary `~ !` and parentheses
  (stage 77; extended stage 99).
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
- **File-scope integer-typed global initializers accept full compile-time
  constant expressions** — the same arithmetic, bitwise, shift, unary,
  and `sizeof(type-name)` operators available in `case` labels and enum
  values. Expressions are evaluated at parse time and stored as
  `AST_INT_LITERAL`; no codegen changes were needed — stage 100.
- File-scope (global) variable declarations (`.bss` / `.data`,
  RIP-relative addressing).
- Storage-class specifiers `extern` and `static` at file scope, **plus
  block-scope `static` local variables** that persist across calls and
  are emitted to `.bss` (uninitialized) or `.data` (initialized) with
  RIP-relative `[rel Lstatic_func_N]` addressing. Scalar and pointer
  types supported since stage 71; **arrays, structs, and unions** added
  in stage 101.
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

### Integer constant expressions (stage 77; extended stages 99 and 100)

A unified nine-level evaluator (`eval_const_expr`) is used for:
- **`case` labels**: `case 1 << 2:` (evaluates to 4), `case PERM_READ | PERM_WRITE:`.
- **Enumerator values**: `FLAG_READ = 1 << 0`, `STEP = BASE + 5`, `ALL = ~0`,
  `SMALL = (4 * 8)`.
- **Array designator indices**: `[2 + 1] = value`.
- **File-scope integer initializers** (stage 100): `int PAGE = 1 << 12;`,
  `int MASK = 0xF0 | 0x0F;`, `int BUF = sizeof(int) * 256;`,
  `int NEG = -(3 * 4);`.

Operators supported (loosest to tightest): `| ^ & + - << >> * / %`
(division-by-zero → compile error); unary `+ - ~ !`; parenthesized
sub-expressions; integer/character literals; enum constants by name;
**`sizeof(type-name)`** (stage 100 — evaluates via `type_size(t)`).

### File-scope constant expressions (stage 100)

Integer-typed global variable initializers now accept the full
`eval_const_expr` evaluator. Pointer-typed and struct/union-typed globals
retain literal-only initialization.

```c
int PAGE_SIZE = 1 << 12;              /* shift          */
int MASK      = FLAG_A | FLAG_B;      /* bitwise OR     */
int BUF_SZ    = sizeof(int) * 1024;  /* sizeof in expr */
int NEG       = -(3 * 4);            /* arithmetic     */
int A = 1 << 2, B = 3 * 7;           /* multi-decl     */
```

Additionally, `sizeof(type-name)` is now supported in any
constant-expression context (enum values, `case` labels, array
designator indices, and file-scope initializers), and a pre-existing
bug that rejected `sizeof(void *)` in general expressions is fixed.

### Block-scope static aggregates (stage 101)

Block-scope `static` local variables can now hold array, struct, and
union types:

```c
void histogram(int bucket) {
    static int counts[8];              /* .bss — zero-initialized */
    static int totals[4] = {0,0,0,0}; /* .data — initialized     */
    counts[bucket]++;
    totals[bucket % 4]++;
}

void origin_demo(void) {
    static struct Point org = {1, 2}; /* .data — initialized */
    org.x++;
}
```

Brace-enclosed initializers (for arrays and structs) and string-literal
initializers (for `char` arrays) are accepted. The initializer must be
a compile-time constant list (runtime expressions rejected). Uninitialized
aggregate statics go to `.bss`; initialized ones go to `.data`. All access
uses RIP-relative `[rel Lstatic_func_N]` addressing, just like file-scope
globals.

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
- `va_copy` is recognized but its codegen is still a no-op stub.

### Expressions
- Integer (decimal/hex), string, and character literals; adjacent
  string-literal concatenation; hex/octal escapes; variable references.
- All eleven assignment operators on any modifiable lvalue.
- Arithmetic, bitwise, shift, equality/relational, and logical operators.
- Pointer arithmetic including difference (`long`).
- Casts; integer promotions and UAC.
- `sizeof(type)` and `sizeof expr` (operand not evaluated); `sizeof(void *)` now works.
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
- Block-scope static aggregates with RIP-relative addressing.
- Per-file codegen teardown; dynamic Vec/StrBuf storage (no fixed caps).

### Tooling & diagnostics
- `--version`, `--print-ast`, `--print-tokens`, `--print-asm` modes.
- `--max-errors=N` multiple-error collection with parser recovery.
- Multi-mode build workflow (`build.sh`): `--mode=normal` / `bootstrap` /
  `fallback` / `self-host` (C0→C1→C2 with test runs and checkpoint commits).
- Strict ISO C99 source; CMake build with `-Wall -Wextra -pedantic`.

## Stage-by-Stage Timeline (76–101)

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
| 100        | File-scope constant expressions + `sizeof` in const exprs     |
| **101**    | **Block-scope static arrays, structs, and unions (current)**  |

## Recently Shipped (Stage 101)

**Stage 101 — block-scope static aggregates.**

Block-scope `static` local variables previously supported only scalar and
pointer types (since stage 71). Stage 101 extends codegen to handle
`TYPE_ARRAY`, `TYPE_STRUCT`, and `TYPE_UNION`. The parser already accepted
the declarations; only `include/codegen.h` and `src/codegen.c` changed.

Six targeted codegen edits:
1. `LocalStaticVar` in `include/codegen.h` gains an `ASTNode *init_node`
   field to carry brace-list and string-literal initializers.
2. The guard in `codegen_statement`'s SC_STATIC arm that called
   `compile_error("...not yet supported")` is replaced by new array and
   struct/union registration blocks that generate `Lstatic_func_N` labels,
   register the variable in `cg->locals` with `is_static=1`, and push a
   `LocalStaticVar` with `init_node` set.
3. `codegen_emit_local_statics` is extended: the `.data` loop gains branches
   for char-array-from-string-literal (inline `db`), array-from-brace-list
   (slots-map pattern reusing `MAX_ARRAY_ELEMS_DESIGNATED`), struct
   (delegates to `emit_global_struct`), and union (inline first-member logic);
   the `.bss` loop gains branches for `resx length` (arrays) and
   `resb size` (structs/unions).
4. The `TYPE_ARRAY` branch in `codegen_expression` VAR_REF gains an
   `is_static` guard → `lea rax, [rel label]`.
5. `emit_array_index_addr` gains the same `is_static` guard; a stale
   comment claiming array statics were impossible is removed.
6. `emit_member_addr`'s local-struct branch gains an `is_static` guard →
   `lea rax, [rel label + offset]`.

**Test counts:** 6 new valid tests and 2 new invalid tests. All 1552 tests pass.

The self-host C0→C1→C2 cycle passed with no new issues. The compiler's own
source uses `static` scalars and pointers extensively but no `static`
array or struct locals, so no compiler source changes were needed.

## Out of Scope (Not Yet Implemented)

- Floating-point types (`float`, `double`) and all FP
  arithmetic/literals/conversions.
- `va_arg` for floating-point and struct-by-value types; `va_copy`
  codegen (still a no-op stub).
- Multi-character constants (`'ab'`); wide-character literals.
- Compound literals at file scope.
- **Designated initializers in block-scope static arrays** (plain
  sequential brace-lists only; `[i] = v` inside a static local array
  initializer is rejected).
- **Multidimensional static arrays** (`static int grid[3][4]`) and
  **static arrays of structs** (`static struct P pts[4]`).
- Bit-fields, flexible array members.
- `volatile` code-generation semantics (currently parsed and tracked only);
  `restrict` qualifier.
- Chained designators (`.a.b`, `.arr[0]`); designated union init for
  non-first members.
- Functions returning function pointers; pointer-to-array declarators
  (`(*p)[10]`); old-style (K&R) function definitions; `__attribute__`
  specifiers.
- `#pragma` (including `#pragma once`); `#elifdef` / `#elifndef`.
- GNU variadic macro extensions (`__VA_OPT__`, named variadic args,
  comma deletion).
- Object-file (`.o`) emission and separate linking; header-only precompilation.

## Architecture

The compiler is organized as six source modules:

| Module | Role |
|--------|------|
| `src/lexer.c` | Tokenizer (with preprocessing pipeline) |
| `src/parser.c` | Recursive-descent parser → AST; constant-expression evaluator |
| `src/codegen.c` | Single-pass AST → NASM x86_64 Intel-syntax emitter |
| `src/ast.c` | AST node construction; dynamic children array |
| `src/type.c` | Type system (build, size, alignment, compatibility) |
| `src/util.c` | Error reporting; file I/O; string interning |
| `src/ast_pretty_printer.c` | `--print-ast` mode |
| `src/compiler.c` | Driver: multi-file loop, CLI parsing, teardown |
| `src/vec.c` | Generic growable-array (`Vec`) container |
| `src/strbuf.c` | Dynamic string buffer (`StrBuf`) |
| `src/preprocessor.c` | C preprocessor (macro expansion, conditionals, includes) |
| `src/version.c` | Version string builder |

## Process

- **One spec per stage** in `docs/stages/` — each stage is atomic.
- **Kickoff → Implementation → Tests → Milestone/Transcript** workflow.
- Tests added at each stage; full suite run before closing.
- Self-host C0→C1→C2 cycle (`./build.sh --mode=self-host`) verified at
  each stage since stage 92.
