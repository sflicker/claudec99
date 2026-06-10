# ClaudeC99 Project Status — Through Stage 98

_As of 2026-06-10 (commit `2876336`)_

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
lexer-owned string pool. Stage 95-12 closes the refactor — the preprocessor
`#if` unary-operator buffer becomes a `StrBuf` and the per-switch case/default
label table becomes a `Vec` — so there are now no unchecked fixed-capacity
writes anywhere and no hard cap on case/default labels per switch.

**Stage 96** adds **multi-file compilation**: the driver (`src/compiler.c`)
now accepts one or more positional source-file arguments, compiling each
through a fresh Lexer/Parser/CodeGen/AST cycle with full per-file heap
teardown (`parser_free`, `codegen_free`, `lexer_free`, `ast_free`). A new
`reset_error_state()` helper zeroes the per-file error counters between files,
and string ownership in codegen is consolidated via the new `Vec owned_strings`
field and `codegen_intern()` helper. No language features changed.

**Stage 97** adds **designated initializers** (C99 §6.7.8): both
`.member = value` member designators (for struct/union initializers) and
`[index] = value` array index designators. A new `parse_initializer_element()`
helper dispatches the leading `.` or `[` and produces `AST_DESIGNATED_INIT`
nodes; array indices are evaluated via `eval_case_const_expr` (the same
constant-expression evaluator used for `case` labels). Codegen supports
designated init at all four init sites — local struct, local array, global
struct, and global array — using a cursor-based approach for local contexts
and a `slots[]` map for global contexts (no VLAs, for self-host compatibility).
Local arrays use a two-phase approach: unconditional zero-fill followed by
cursor-driven initializer application. Chained designators (`.a.b`, `.arr[0]`)
and designator/context mismatches are diagnosed.

**Stage 98** adds **compound literals** (C99 §6.5.2.5): the `(type-name){ initializer-list }`
syntax creates unnamed temporary objects on the stack. Both struct/union and
array compound literals are supported, along with scalar compound literals
(`(int){ 7 }`). Array compound literals may omit the first dimension to
infer the length from the initializer list. Compound literals can be passed
directly to functions, assigned to variables, used as postfix bases for `.field`
and `[index]` access, and have their address taken with `&`. `AST_COMPOUND_LITERAL`
nodes carry the type label, base kind, pointer to the type, and an
rbp-relative stack offset assigned by a pre-scan phase
(`scan_expr_compound_literals`) before the function prologue is emitted.
Compound literals at file scope are detected and rejected with a specific
diagnostic. A new `parse_postfix` helper saves/restores lexer state to detect
`(type-name){` before falling back to `parse_primary`, ensuring unary operators
like `&` work on compound literals.

**Stages completed: 205** (stage-01 through stage-98, including substages).

## Build & Test

| Component                     | Count          |
|-------------------------------|----------------|
| Source files (`src/*.c`)      | 12             |
| Header files (`include/`)     | 13             |
| Total LOC (src + include)     | 14,496         |
| Valid runtime tests           | 855            |
| Invalid (compile-error) tests | 246            |
| Integration tests             | 86             |
| Print-AST golden tests        | 48             |
| Print-tokens golden tests     | 100            |
| Print-asm golden tests        | 21             |
| **Total tests**               | **1,521**      |
| Git commits                   | 801            |

All 1,521 tests pass with no regressions — under the gcc-built compiler and
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
  pipeline with full per-file heap teardown (`parser_free`, `codegen_free`,
  `lexer_free`, `ast_free`, `reset_error_state`) — stage 96.

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
  RIP-relative `[rel Lstatic_func_N]` addressing. (Block-scope `static`
  *arrays* are not yet supported.)
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
- Enum declarations with compile-time constant folding; `NULL`.

### Designated initializers (C99 §6.7.8)
- **`.member = value`** member designators in struct brace initializers
  at both local and global scope; the cursor jumps to the named field and
  subsequent non-designated elements continue from that field onward.
- **`[index] = value`** array index designators at both local and global
  scope; the index must be a non-negative compile-time constant expression
  (evaluated by `eval_case_const_expr`); local arrays are zero-filled
  first then overwritten at designated positions.
- Mixing designated and non-designated elements in the same initializer
  list is supported.
- Chained designators (`.a.b`, `.arr[0]`) and context mismatches
  (member designator in an array initializer or vice versa) are diagnosed.
- Fixed-size arrays are used internally (`MAX_STRUCT_FIELDS_DESIGNATED = 64`,
  `MAX_ARRAY_ELEMS_DESIGNATED = 1024`) for self-host compatibility (no VLAs).

### Compound literals (C99 §6.5.2.5)
- **Struct and union compound literals**: `(struct Point){ .x = 1, .y = 2 }`
  creates a named-field aggregate on the stack, which can be passed to
  functions, assigned to a struct variable, or have its address taken.
- **Array compound literals**: `(int[]){ 1, 2, 3 }` (omitted-size — length
  inferred from initializer), `(int[4]){ 1, 2, 3 }` (explicit size), and
  `(int[5]){ [3] = 10 }` (with index designators). An omitted-size array
  compound literal uses `infer_compound_literal_array_length` to compute
  the length from the initializer list.
- **Scalar compound literals**: `(int){ 7 }` creates a modifiable scalar
  lvalue on the stack; `&(int){ 7 }` yields a pointer to it.
- **Postfix chaining**: `(struct Point){ .x = 0, .y = 1 }.y` (member access)
  and `(int[]){ 10, 20, 30 }[1]` (subscript) are supported via
  `parse_postfix_suffixes`.
- **Address-of**: `&(T){ … }` is supported for all compound literal types.
  Struct/union and array literals already carry their address in `rax`
  after codegen; scalar literals emit `lea rax, [rbp - offset]`.
- Stack allocation: a two-phase pre-scan (`compute_compound_literal_bytes`
  for the frame-size estimate; `scan_expr_compound_literals` for actual
  offset assignment) assigns each compound literal an rbp-relative stack
  slot before the function prologue is emitted.
- Compound literals at file scope are detected and rejected: "compound
  literals at file scope are not yet supported".

### Struct/union by value (System V AMD64 ABI)
- **Struct and union parameters and return values passed by value**,
  classified per the SysV AMD64 ABI: register-class aggregates (≤16 bytes)
  travel in `rax`/`rdx` and the integer argument registers; memory-class
  aggregates (>16 bytes) travel through a hidden pointer (`rdi` for
  returns).
- A shared call-layout helper drives both the call site and the prologue.
  Codegen adds `emit_struct_addr` (field addresses), `emit_struct_copy`
  (block copy via `rep movsb`), and `compute_struct_ret_bytes` /
  `claim_struct_ret_temp` (struct-return scratch space); the codegen
  context tracks `struct_ret_scratch_base`, `struct_ret_scratch_cursor`,
  and `current_sret_offset`.
- Whole-struct assignment and declaration-initialization accept struct
  rvalues from a variable, a function return, or an explicit copy —
  including the subscript (`arr[i] = f()`), dot (`obj.m = f()`), arrow
  (`p->m = f()`), and deref (`*p = f()`) target forms.
- **Compound literals as struct values**: `(struct T){ … }` compound
  literals can be passed as struct arguments and assigned to struct
  variables (stage 98).
- **Not yet supported**: `va_arg` of struct-by-value type.

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
  references. `sizeof` of a string literal yields its length+1.
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
  multidimensional array types and arrow/subscripted member arrays.
- **Address-of (`&`) on any addressable lvalue** — variables, subscripts,
  member/arrow access, and **compound literals** (`&(T){ … }`);
  dereference (rvalue and lvalue); subscript; multi-level subscript;
  member access `.` / `->`.
- **Compound literals** `(type-name){ … }` — struct, union, array, and
  scalar; postfix `.field` / `[index]` chaining; usable as function
  arguments; address-takeable.
- Function calls with **any number of arguments** (≤6 in registers, 7+
  via SysV stack-passing); variadic calls; **struct/union arguments and
  return values by value**; indirect calls; `void *` implicit
  conversions; the null constant `0` accepted as a pointer argument.

### Code generation
- Single-pass walk from AST to NASM Intel-syntax assembly; per-function
  stack frames with natural alignment.
- Integer promotions, UAC, and LP64-aware conversions at every binary op.
- `movzx`/`movsx` sized loads; `div`/`idiv`; `shr`/`sar`; unsigned
  comparison instructions; `_Bool` normalization.
- String literals in `.rodata`; struct/union member addressing and
  block struct copies (`rep movsb`); `.data`/`.bss` globals, `extern`
  object directives, `global` directives for non-static definitions, and
  block-scope statics with RIP-relative access.
- Struct/union by-value marshalling at call sites and prologues per the
  SysV AMD64 classification (register-class vs memory-class), with a
  shared call-layout helper and hidden-pointer struct returns.
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
- The AST node `children` array is a lazily-allocated doubling array
  (stage 92), so large translation units, blocks, and switches are no
  longer silently truncated at 64 children.
- Since the 95-xx refactor every parser/codegen symbol table is a dynamic
  `Vec` (locals, globals, string pool, enum constants/tags, struct/union
  tags, typedefs, functions, break/switch stacks, user labels, struct-field
  scratch) rather than a fixed array, and all name/tag/label text is a
  `const char *` into a lexer-owned string pool rather than a
  `char[MAX_NAME_LEN]` buffer — so there are no hard caps on table sizes or
  identifier/literal lengths, and the old 255-byte string-literal limit is
  gone.
- **Per-file codegen teardown (stage 96)**: `codegen_free()` frees all
  codegen-owned strings (tracked in `Vec owned_strings` via
  `codegen_intern()`) and all eight `Vec` fields; `parser_free()` frees
  the seven parser `Vec`s; `reset_error_state()` zeroes the per-file error
  counters. The driver loops over source files, calling these after each.
- **Designated initializer codegen (stage 97)**: `AST_DESIGNATED_INIT`
  nodes are handled at four sites. Local struct init uses field-name lookup
  and a cursor that advances to the named field; local array init uses
  two-phase logic (unconditional zero-fill then cursor-driven overwrites);
  global struct emit uses a `slots[MAX_STRUCT_FIELDS_DESIGNATED]` map to
  assign values by field index before emitting in declared order; global
  array emit uses a `slots[MAX_ARRAY_ELEMS_DESIGNATED]` map similarly.
- **Compound literal codegen (stage 98)**: a two-phase pre-scan
  (`compute_compound_literal_bytes` / `scan_expr_compound_literals`)
  assigns each `AST_COMPOUND_LITERAL` node an rbp-relative stack offset
  before the function prologue; the frame-size estimator accounts for their
  sizes. At emission time, struct/union compound literals delegate to
  `emit_local_struct_init`; array compound literals delegate to the local
  array initializer path; scalar compound literals store directly to their
  stack slot. `emit_struct_addr` handles `AST_COMPOUND_LITERAL` to support
  struct-value contexts. `AST_ADDR_OF` on a compound literal emits either
  the already-loaded address (struct/union/array) or `lea rax, [rbp -
  offset]` (scalar).

### Tooling & diagnostics
- `--version` reports a two-line build identifier: a
  `MM.mm.SSSSSSSS.BBBBB` version string (build number sourced from
  `git rev-list --count`) plus a `BuiltBy:` line naming the compiler that
  built it (`VERSION_BUILTBY`, defaulting to `DefaultCcompiler`, computed
  by CMake from the compiler ID/version).
- **Multi-mode build workflow** (`build.sh`, stages 93/94): `--mode=normal`
  (cmake), `--mode=bootstrap` (self-compile via a pre-built ccompiler with
  a per-file `--timeout` guard, default 300s), `--mode=fallback` (bootstrap
  if available, else normal), and `--mode=self-host` (full C0→C1→C2 cycle
  with a fresh gcc-built C0, test runs, and a commit checkpoint at each
  step — stage 94). Test runners honor `CLAUDEC99_TEST_TIMEOUT` (default
  30s) and wrap compiler/program invocations in `timeout`.
- `--max-errors=N` collects multiple diagnostics before aborting
  (default 1 = exit on first error); parser recovers via setjmp/longjmp
  and `parser_sync()` to the next declaration boundary.
- **Multi-file error isolation**: with `--max-errors=0`, a compile error
  in one file does not terminate the process — the remaining files are
  still compiled and their output produced (stage 96).
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

## Stage-by-Stage Timeline (76–98)

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
| **98**     | **Compound literals `(T){ … }` — struct, array, scalar (current)** |

## Recently Shipped (Stage 98)

**Stage 98 — compound literals.**

C99 §6.5.2.5 compound literals are now supported: `(type-name){ initializer-list }`
creates an unnamed temporary object on the stack that is a modifiable lvalue.

The parser changes are contained in two places. In `parse_cast`, after
parsing `(type-name)`, the next-token check now also tests for `TOKEN_LBRACE`
— if found, the type is dispatched to `build_compound_literal` instead of
the regular cast path. `TOKEN_STRUCT` and `TOKEN_UNION` are added to the
type-start detection. A second detection point was added in `parse_postfix`:
the function saves lexer state before `(`, peeks at the following token, and
if `(type-name){` is confirmed, builds the compound literal there. If
`(type-name)` appears but `{` does not follow, the saved lexer state is
restored and `parse_primary` is called instead — this ensures that unary
operators like `&` on compound literals work correctly (the `parse_unary`
→ `parse_postfix` path would have seen `(int)` and called `parse_expression`
inside, which cannot start with a keyword).

`build_compound_literal` handles four cases:
- **struct/union**: calls `parse_initializer` for the brace body.
- **array with explicit length N**: calls `parse_initializer`.
- **array with omitted first dimension** (`(int[]){ … }`): calls
  `parse_initializer`, then walks the resulting `AST_INITIALIZER_LIST` via
  `infer_compound_literal_array_length` to count elements (tracking any
  `[N]` designator repositions), and rebuilds `full_type` with the inferred
  length.
- **scalar** (`(int){ 7 }`): consumes `{`, calls `parse_assignment_expression`,
  allows a trailing comma, consumes `}`.

`parse_type_name` was updated to accept `[]` (omitted first dimension) in
array-suffix position specifically for compound literals; `sizeof(T[])` and
cast `(T[])expr` with an omitted size still produce errors.

Stack allocation uses a two-phase pre-scan:
- `compute_compound_literal_bytes` walks the AST to estimate the bytes needed
  for all compound literals in a function (used to size the stack frame).
- `scan_expr_compound_literals` makes a second pass, assigning each
  `AST_COMPOUND_LITERAL` node a concrete `rbp`-relative offset (stored in
  `node->byte_length`).

Both scans are called before the NASM prologue is emitted so the frame size
is known.

Four codegen sites were updated or added:
- **Struct/union compound literals**: `codegen_expression` for
  `AST_COMPOUND_LITERAL` calls `emit_local_struct_init` with the stack
  address; `emit_struct_addr` handles the `AST_COMPOUND_LITERAL` case (the
  codegen overwrites `node->full_type` to `type_pointer(st)` so `emit_struct_addr`
  returns `node->full_type->base` — the original struct type — to the struct
  init checker).
- **Array compound literals**: delegates to the existing local array
  initializer path.
- **Scalar compound literals**: stores the value directly to `[rbp - offset]`.
- **`AST_ADDR_OF` on a compound literal**: struct/union and array literals
  already have their address in `rax` from codegen; scalar literals emit
  `lea rax, [rbp - offset]`. The result type is set to a pointer to the base type.

20 new tests were added: 12 valid (struct-arg pass, struct-assign, array
explicit, array omitted size, array with designator, postfix member, postfix
subscript, address-of, scalar, zero-fill, dot-product, char array), 4 invalid
(file scope, void type, too many initializers, out-of-bounds designator), 3
print-AST golden tests (struct, array, scalar), and 1 integration test
(`test_compound_literal_multifile` with two source files — one struct and one
array compound literal used across a multi-file build).

The self-host C0→C1→C2 cycle passed with no new issues.

## Out of Scope (Not Yet Implemented)

- Floating-point types (`float`, `double`) and all FP
  arithmetic/literals/conversions.
- `va_arg` for floating-point and struct-by-value types; `va_copy`
  codegen (still a no-op stub).
- Multi-character constants (`'ab'`); wide-character literals.
- Compound literals at file scope.
- Block-scope `static` arrays.
- Bit-fields, flexible array members.
- `volatile` code-generation semantics (currently parsed and tracked only);
  `restrict` qualifier.
- `typedef enum`; enum constant expressions beyond integer/character
  literals and `+`/`-` folding.
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
in `docs/other/stage-98-parse-tree.md`, and the feature checklist in
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
