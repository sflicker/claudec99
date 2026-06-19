# ClaudeC99 Project Status — Through Stage 138

_As of 2026-06-18 (commit `f957b8e`)_

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

**Stages 100–104** complete the **compile-time constant expression** subsystem.
Stage 100 wires `eval_const_expr` into file-scope integer global initializers
and adds `sizeof(type-name)` support to the evaluator. Stage 101 lifts codegen
restrictions on block-scope static arrays, structs, and unions. Stage 102 adds
designated initializers, struct/union elements, and 2D arrays for static
aggregates. Stage 103 adds `eval_const_init` (an AST-based evaluator) for
block-scope static scalar initializers. Stage 104 extends both evaluators with
relational, equality, logical, and ternary operators — completing the full
C99 integer constant expression operator set.

**Stage 105** adds `#pragma` (unknown pragmas silently ignored; `#pragma once`
supported), `_Pragma`, `#line`, and the null directive to the preprocessor.

**Stage 106** adds the `restrict` qualifier (parse-and-ignore), a comprehensive
stub header rewrite for `ctype.h`, `string.h`, `stdlib.h`, and `stdio.h`,
and a bug fix for `TYPE_LONG_LONG` in function-return sign-extension.

**Stage 107** adds the `inline` keyword (parse-and-ignore), a functional
`va_copy` codegen (three 8-byte `rax` moves), and a `test/include/assert.h`
stub with NDEBUG-aware `assert`.

**Stage 108** adds `#elifdef` and `#elifndef` preprocessor directives.

**Stages 109–112** add **floating-point support**: `float`/`double` types,
literals, stack/global variables (109); FP arithmetic, UAC, and casts (110);
FP comparisons and boolean contexts (111); the full SysV AMD64 FP calling
convention with XMM registers, `va_arg` for `double`, and a `math.h` stub
(112).

**Stages 113–120** are test suite and bug-fix stages: test suite reorganization
(113); legacy test integration with 7 codegen bug fixes (114); constant
expressions in array dimension bounds (115); global struct array BSS fix and
`char[N]` string initialization (116); FP struct member rvalue read fixes (117);
pointer relational operators (118); FP compound assignment on global struct
members (119); FP `++`/`--` on struct members (120).

**Stage 121** fixes `switch` for 64-bit discriminants (`long`, `long long`,
`unsigned long long`).

**Stage 122** adds `rbx` callee-saved preservation per the SysV AMD64 ABI.

**Stage 123** fixes five ABI correctness bugs: FP stack arguments, indirect
FP calls, address-constant global initializers, and FP logical short-circuit.

**Stage 124** adds octal integer literals, the `__func__` predefined identifier
(C99 §6.4.2.2), and file-scope compound literals for pointer types.

**Stage 125** fixes FP global initialization from integer constants and enforces
variadic float→double promotion per C99 §6.5.2.2p7.

**Stage 126** implements tentative definitions (C99 §6.9.2) — multiple
file-scope declarations of the same variable without initializers merge; the
first definition with an initializer resolves the group.

**Stage 127** adds callee-saved preservation for r12–r15, completing full
SysV AMD64 ABI compliance for all five callee-saved GP registers (rbx, r12–r15).

**Stage 128** adds a recursive-descent FP constant expression evaluator
(`eval_fp_const_expr`) for file-scope float/double global initializers, enabling
`double x = 3.14159 * 2.0;` evaluated at compile time.

**Stage 129** lifts two syntactic restrictions: block-scope function declarations
(`void foo(void);` inside `{}`) and extern incomplete array declarations
(`extern int arr[];` before the defining declaration).

**Stage 130** implements `va_arg` for struct/union by value via the full SysV
AMD64 three-case classification (MEMORY class >16 bytes, 1-eightbyte register
class, 2-eightbyte register class). This closes the last major gap in variadic
function support.

**Stage 131** fixes `sizeof` to produce an **unsigned `size_t` result** per C99
§6.5.3.4. A two-line codegen fix sets `node->is_unsigned = 1` on both
`AST_SIZEOF_TYPE` and `AST_SIZEOF_EXPR` nodes, so that comparisons like
`sizeof(int) > -1` are correctly true and arithmetic like `sizeof(int) - 2`
uses unsigned semantics.

**Stage 132** relaxes **pointer equality** to accept non-zero integer constant
operands, matching GCC/Clang extension behavior. The `is_null_pointer_constant()`
guard in the pointer equality validation is replaced by `is_integer_constant()`
(accepting any `AST_INT_LITERAL`). Non-constant integer expressions remain
rejected with an updated error message.

**Stage 133** ships C99 §6.5.2.2p6 **default argument promotions** for calls to
functions declared without a prototype (`int f()`). The parser now distinguishes
an empty `()` parameter list ("no prototype information", `ASTNode.is_no_prototype = 1`)
from `(void)` (zero-parameter prototype). At call sites, codegen applies
float→double promotion when `is_variadic || is_no_prototype`; char/short→int
promotion is automatic via existing `movsx`/`movzx` load instructions.

**Stage 134** ships **bit-field struct members** (CC99-006) and **flexible array
members** (CC99-007). `parse_struct_specifier` is restructured to detect `:width`
declarators (named and anonymous bit-fields) and unsized trailing array members
(`type name[]`). Bit-fields pack into GCC-compatible storage units with
shift+mask rvalue reads and read-modify-write lvalue writes. Flexible array
members contribute zero bytes to `sizeof`; existing array-decay codegen handles
indexed access. `StructField` in `include/type.h` gains four new fields:
`is_bitfield`, `bit_width`, `bit_offset`, and `is_flexible_array`.

**Stage 135** fixes two C99 type compatibility bugs affecting function parameters
(CC99-008, CC99-009). `parse_parameter_declaration` now applies array-to-pointer
(C99 §6.7.5.3p7) and function-to-pointer-to-function (C99 §6.7.5.3p8)
adjustments for named declarators before compatibility checks, so `int values[3]`,
`int values[]`, and `int *values` form compatible function types. `parse_declarator`
now parses `(*name)[N]` **pointer-to-array** declarators instead of rejecting
them; `ParsedDeclarator` gains `is_ptr_to_array`, `ptr_to_array_length`, and
`ptr_to_array_has_size` fields. Fixed a pre-existing bug where `(void)` in
function-pointer parameter lists was counted as one void parameter rather than zero.

**Stage 136** fixes a bug in `sizeof_type_of_expr` where `sizeof` applied to
pointer-arithmetic expressions returned the element size instead of the pointer
size. Two codegen fixes: (1) an `AST_BINARY_OP` case for `+`/`-` with pointer/
array operands now returns `TYPE_POINTER` (8 bytes on LP64); (2) an
`AST_STRING_LITERAL` case returns `TYPE_POINTER` so string literals in binary
expressions are treated as pointers.

**Stage 137** ships **functions returning function pointers** (`(*get_adder())(int)`
declarator form), resolving CC99-010. `parse_declarator` now parses the
`(*name())(params)` form fully; `ParsedDeclarator` gains `is_func_returning_fp`,
`own_param_types`, `own_param_count`, and `own_is_no_prototype` fields. In
`parse_external_declaration`, an `is_func_returning_fp` branch constructs the
nested pointer-to-function return type, creates `AST_FUNCTION_DECL` with
`decl_type = TYPE_POINTER`, registers the function, and optionally parses its
body. Fixed a pre-existing bug where typedef'd pointer return types were not
recognized via the `func->full_type` assignment condition. No codegen changes —
the function is called and returned from identically to any other
pointer-returning function.

**Stage 138** adds `auto` and `register` storage-class specifiers (CC99-011,
CC99-012). New tokens `TOKEN_AUTO` and `TOKEN_REGISTER` are recognized by the
lexer. `parse_declaration_specifiers` rejects both at file scope.
`parse_statement` handles `auto` (treated as default automatic storage,
`SC_AUTO = 8`) and `register` (`SC_REGISTER = 16`) at block scope.
`parse_parameter_declaration` accepts `register` as a leading qualifier.
`SC_AUTO` and `SC_REGISTER` added to the `StorageClass` enum; `is_register`
added to `LocalVar`; `AST_ADDR_OF` rejects address-of on register variables
with a compile error.

**Stages completed: 245** (stage-01 through stage-138, including substages).

## Build & Test

| Component                     | Count          |
|-------------------------------|----------------|
| Source files (`src/*.c`)      | 12             |
| Header files (`include/`)     | 13             |
| Total LOC (src + include)     | 18,041         |
| Valid runtime tests           | 1,284          |
| Invalid (compile-error) tests | 262            |
| Integration tests             | 88             |
| Print-asm golden tests        | 21             |
| Print-AST tests               | 50             |
| Print-tokens tests            | 100            |
| Unit tests                    | 165            |
| **Total tests**               | **1,970**      |
| Git commits                   | 1,038          |

All 1,970 tests pass with no regressions — under the gcc-built compiler and
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
- **`#elifdef` / `#elifndef`** (C23 / GCC extension; stage 108).
- **Preprocessor conditional expressions**: full evaluator for
  `#if` / `#elif` supporting integer literals, `defined(NAME)`,
  object-like macro expansion, undefined-identifier-as-0, unary
  `!` / `-` / `+` / `~`, parentheses, `==` / `!=` / `<` / `<=` /
  `>` / `>=`, `&&` / `||`, `+` / `-` / `*` / `/` / `%`, `&` / `^` /
  `|`, `<<` / `>>`.
- **`#error message`**.
- **`#pragma`**: unknown pragmas silently ignored; `#pragma once`
  supported (stage 105). `_Pragma("string")` operator destringizes
  and applies as a pragma.
- **`#line N "file"`**: overrides `__LINE__` / `__FILE__` for subsequent
  expansions (stage 105).
- **Null directive**: bare `#` on a line silently consumed (stage 105).
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
  standard streams `stdin` / `stdout` / `stderr` (extern `FILE *`),
  file-position / read stubs `fseek` / `ftell` / `fread` with
  `SEEK_SET` / `SEEK_CUR` / `SEEK_END`, and the full set of C99 stdio
  functions (stage 106 rewrite).
- **`stdlib.h`** — `malloc`, `calloc`, `free`, `realloc`, `exit`,
  `strtol`, `strtoul`, `strtod` (stage 131 bootstrap fix), plus `div_t`/
  `ldiv_t`/`lldiv_t`, `EXIT_SUCCESS`/`EXIT_FAILURE`/`RAND_MAX`, and the
  full stage-106 function set.
- **`string.h`** — `strlen`, `strcmp`, `strcpy`, `memcpy`, `memset`,
  `memcmp`, `strchr`, `strncat`, `strncmp`, `strncpy`, `strrchr`, plus
  the stage-106 additions (`memmove`, `strstr`, `strtok`, etc.).
- **`stddef.h`** — `NULL`, `size_t`, `ptrdiff_t`.
- **`stdint.h`** — exact-width, pointer-size, fast, and least integer
  typedefs.
- **`limits.h`** — full set including `LLONG_MIN`, `LLONG_MAX`,
  `ULLONG_MAX`.
- **`stdbool.h`** — `bool`, `true`, `false`.
- **`ctype.h`** — character classification (`isalpha`, `isdigit`,
  `isspace`, …), plus stage-106 additions.
- **`errno.h`** — `errno`, `ERANGE`, `EINVAL`, etc.
- **`time.h`** — `time_t` and time functions.
- **`setjmp.h`** — non-local jump support.
- **`stdarg.h`** — `va_list` typedef and the `va_start` / `va_end` /
  `va_copy` / `va_arg` macros (expanding to `__builtin_va_*`).
- **`math.h`** — `double` and `float` variants of `sin`, `cos`, `sqrt`,
  `pow`, `fabs`, `floor`, `ceil`, and more; `M_PI`, `M_E`, `M_SQRT2`
  constants (stage 112).
- **`assert.h`** — NDEBUG-aware `assert` macro using `fprintf` and
  `abort` (stage 107).

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
  with unary `~ !`, relational/equality/logical/ternary operators, and
  parenthesized sub-expressions (stages 77, 99, 104).
  **`switch` on `long`/`long long`/`unsigned long long` discriminants** emits
  64-bit `cmp rax` comparisons (stage 121).
- `break`, `continue`, `goto` + labeled statement.
- Compound block statements with lexical scoping and shadowing.
- Expression statements.

### Declarations & types
- Integer base types `char`/`short`/`int`/`long`, all unsigned variants,
  `long long` / `unsigned long long`, the `signed` keyword forms, and
  trailing-`int` forms.
- **`float` and `double`** scalar types with full arithmetic, casts,
  comparisons, boolean contexts, struct members, globals, and FP calling
  convention (stages 109–112; see Floating-point below).
- `_Bool` with value-normalization and integer promotion.
- Integer literal forms: decimal, **`0x`/`0X` hexadecimal**, and **`0NNN`
  octal** literals (stage 124; decimal-rewritten in lexer for NASM),
  with suffixes `U`/`L`/`UL`/`LL`/`ULL` and overflow-aware typing.
- Floating-point literal forms: decimal with `.`/exponent/`f`/`F` suffix;
  leading-dot form (`.5f`); both `TOKEN_FLOAT_LITERAL` and
  `TOKEN_DOUBLE_LITERAL` (stage 109).
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
- **`restrict` qualifier**: parsed and discarded at all pointer-qualifier
  positions (stage 106); no semantic or codegen effect.
- **`inline` keyword**: parsed and discarded (stage 107).
- **`auto` storage class** (stage 138): explicit `auto` at block scope
  is accepted and treated identically to plain automatic storage (`SC_AUTO`).
- **`register` storage class** (stage 138): `register` at block scope and
  in parameter declarations is accepted; the compiler enforces no-address-of
  at compile time (`AST_ADDR_OF` rejected on register variables); otherwise
  generates identical code to automatic declarations.
- Pointer types of arbitrary depth; array types; **multidimensional
  arrays** (declarations, indexing, member access, and `sizeof`, up to
  `MAX_ARRAY_DIMS = 8`); multi-level subscript.
- **Pointer-to-array declarators** (stage 135): `int (*row)[4]` and
  `int (*row)[]` are now valid declarations; indexed access `(*row)[i]`
  works via existing deref + subscript codegen.
- **Array dimension constant expressions** (stage 115): dimension bounds
  accept full integer constant expressions (`int a[N*2]`,
  `int a[sizeof(int)*8]`, macro identifiers, etc.).
- Function pointer types in local, parameter, file-scope, static, and
  extern positions; indirect calls.
- **Functions returning function pointers** (stage 137): `int (*f())(int)`
  declarator form fully supported; the function is registered with a
  `TYPE_POINTER` return type and called/returned identically to any
  other pointer-returning function.
- Parenthesized and abstract declarators; array-to-pointer decay for
  array-typedef function parameters and for **struct/union array members**
  in value contexts.
- Comma-separated init-declarator lists; brace and string initializers
  (local and file scope, including **char-array struct members from
  string literals**) with size inference; **designated initializers**
  (`.member = value` and `[index] = value`) at local and global scope
  for both arrays and structs — stage 97.
- **Compound literals** (`(type-name){ initializer-list }`) — unnamed
  temporary objects allocated on the stack (stage 98; file-scope
  pointer-type compound literals stage 124).
- File-scope (global) variable declarations (`.bss` / `.data`,
  RIP-relative addressing).
- Storage-class specifiers `extern` and `static` at file scope, **plus
  block-scope `static` local variables** (scalars/pointers stage 71;
  arrays/structs/unions stage 101; full constant-expression initializers
  stages 102–103).
- **Tentative definitions** (C99 §6.9.2 — stage 126): multiple file-scope
  declarations without initializers merge; the completing initializer
  resolves the group.
- **Extern incomplete array declarations** (stage 129): `extern int arr[];`
  before the defining declaration is accepted.
- `typedef` aliases for scalar / pointer / array / function-pointer and
  complete struct / union types; block-scope shadowing.
- **Block-scope function declarations** (stage 129): `void foo(void);`
  inside `{}` registers the function with unknown arity (`param_count=-1`).
- Struct definitions, member access, brace initializers (including
  **designated member initializers** — `.field = value` — at local and
  global scope), whole-struct assignment/copy, pointer-to-struct mutation,
  nested structs, arrays of structs, typedef aliases, and incomplete
  forward declarations. Float/double struct members with full SSE2
  read/write/arithmetic support (stages 117–120).
- **Bit-field struct members** (stage 134): named bit-fields (`unsigned int x : N`),
  anonymous bit-fields (`: N` padding), and zero-width bit-fields (`: 0` force
  new storage unit). GCC-compatible packing; shift+mask rvalue reads; RMW
  lvalue writes.
- **Flexible array members** (stage 134): `type name[]` as the last named
  struct member. `sizeof` excludes the flexible member; indexed access works
  via existing array-decay codegen.
- **Named unions** (`union Tag { … }`): layout sized to the largest
  member, member access via `.` / `->`, nested types, whole-union
  assignment, first-member brace initialization, and globals.
- **Anonymous struct/union type declarations** without a tag: each
  definition allocates a fresh unique `Type*`; type identity is by
  pointer, so structurally identical anonymous types are distinct.
- **Enum declarations** with compile-time constant folding; `NULL`.
  Enumerator values accept full integer constant expressions (arithmetic,
  shift, bitwise, unary `~ !`, relational, equality, logical, ternary,
  parentheses, and references to previously-defined enum constants —
  stages 99, 104).
  Forward-declared enum tags (`typedef enum Status Status;` before the
  body) are supported — stage 99.

### Integer constant expressions (stages 77, 99–104, 115)

A unified evaluator (`eval_const_expr`) covers:
- **`case` labels**: `case 1 << 2:`, `case PERM_READ | PERM_WRITE:`.
- **Enumerator values**: `FLAG_READ = 1 << 0`, `STEP = BASE + 5`, `ALL = ~0`.
- **Array designator indices**: `[2 + 1] = value`.
- **Array dimension bounds** (stage 115): `int a[N*2]`, `int a[sizeof(int)*8]`.
- **File-scope integer global initializers** (stage 100): `int x = (1 << 4) | 3;`.
- **Block-scope static scalar initializers** (stage 103): `static int k = A * B + C;`.
- **Bit-field width expressions** (stage 134): `unsigned int x : N`.

Operators: `| ^ & + - << >> * / %` (division-by-zero → compile error);
unary `+ - ~ !`; `< <= > >= == != && || ?:`; parenthesized sub-expressions;
integer/character literals; enum constants by name; `sizeof(type-name)` (stage 100).

### Floating-point (stages 109–112, 117–120, 123, 125, 128)

- **Types**: `float` (4 bytes) and `double` (8 bytes); `TYPE_FLOAT` / `TYPE_DOUBLE`
  singleton types; `type_is_fp()` helper.
- **Literals**: `TOKEN_FLOAT_LITERAL` / `TOKEN_DOUBLE_LITERAL` → `AST_FLOAT_LITERAL`;
  deduplication pool in `CodeGen`; emitted to `.rodata` as `Lfc<N>: dd/dq <value>`.
- **Arithmetic**: add/sub/mul/div with SSE2 (`addss`/`addsd`, `mulss`/`mulsd`, etc.);
  unary minus via `xorps`/`xorpd` with 16-byte-aligned sign mask.
- **UAC / conversions**: `cvtsi2ss`, `cvtsi2sd`, `cvtss2sd`, `cvtsd2ss`,
  `cvttss2si`, `cvttsd2si`; implicit widening in `emit_fp_widen_if_needed`.
- **Comparisons**: `ucomiss` / `ucomisd` with NaN-correct `sete+setnp+and` /
  `setne+setp+or` sequences for `==` / `!=`; unsigned `setb`/`seta` variants
  for `<`/`>`.
- **Boolean contexts**: `emit_fp_bool_to_rax` helper converts FP in xmm0 to 0/1;
  used in `if`/`while`/`for`/`do-while` conditions, ternary conditions, and
  logical NOT.
- **Struct members**: full SSE2 read/write for dot, arrow, and subscript-then-dot
  access; FP compound assignment and `++`/`--` on struct FP fields (stages 117–120).
- **Global variables**: `double g = 2.718;` emits to `.data` as `dq <value>`;
  integer initializers converted to IEEE 754 at compile time (stage 125).
- **FP constant expressions at file scope** (stage 128): `double x = 3.14159 * 2.0;`
  evaluated by `eval_fp_const_expr` at compile time.
- **Calling convention** (stage 112): xmm0–xmm7 for `float`/`double` arguments
  and return values; non-variadic prologues move xmmN params to local slots;
  variadic prologues save xmm0–xmm7 to the 176-byte register-save area.
- **Variadic float→double promotion** (stage 125): per C99 §6.5.2.2p7,
  `float` arguments in variadic calls are promoted to `double` via `cvtss2sd`.
- **No-prototype float→double promotion** (stage 133): per C99 §6.5.2.2p6,
  `float` arguments in calls to no-prototype functions (`int f()`) are also
  promoted to `double` via `cvtss2sd`.

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
- **File-scope pointer-type compound literals** (stage 124).

### Struct/union by value (System V AMD64 ABI)
- **Register-class aggregates** (≤16 bytes) travel in `rax`/`rdx` and
  integer argument registers.
- **Memory-class aggregates** (>16 bytes) travel through a hidden pointer
  (`rdi` for returns / `sret`).
- Whole-struct assignment and declaration-initialization accept struct
  rvalues from any target form (subscript, dot, arrow, deref).

### Variadic functions (`<stdarg.h>`)
- `va_start` / `va_end` / `va_copy` (functional 24-byte struct copy — stage 107).
- `va_arg` for GP-class types (int, long, pointers).
- `va_arg` for `double` via fp_offset / XMM register path (stage 112).
- `va_arg` for `float` rejected with compile error per C99 §6.5.2.2p6 (stage 112).
- **`va_arg` for struct/union by value** (stage 130): SysV AMD64 three-case
  classification (MEMORY class, 1-eightbyte register, 2-eightbyte register);
  `rep movsb` scratch copy.

### Expressions
- Integer (decimal/hex/octal), float/double, string, and character literals;
  adjacent string-literal concatenation; hex/octal escapes; variable references.
- All eleven assignment operators on any modifiable lvalue.
- Arithmetic, bitwise, shift, equality/relational, and logical operators.
- **Floating-point arithmetic**, comparisons, conversions, and UAC (stages 110–111).
- Pointer arithmetic including difference (`long`); **pointer relational
  comparisons** (`<`, `<=`, `>`, `>=`) with unsigned `setb`/`seta` variants
  (stage 118); **pointer equality with non-null integer constants** (stage 132).
- Casts; integer promotions and UAC; FP promotions and UAC.
- **`sizeof(type)` and `sizeof expr`** — operand not evaluated; result is
  **unsigned `size_t`** per C99 §6.5.3.4 (stage 131); correctly sized for
  pointer-arithmetic expressions (stage 136).
- Address-of on any addressable lvalue (register variables excluded — stage 138);
  dereference; subscript.
- Compound literals; conditional (ternary) operator; comma operator.
- Function calls with any number of arguments.
- **`__func__`** predefined string literal (C99 §6.4.2.2 — stage 124).

### Default argument promotions (C99 §6.5.2.2p6, stage 133)
- **No-prototype function declarations** (`int f()`) distinguished from
  zero-parameter prototypes (`int f(void)`); `ASTNode.is_no_prototype` flag set.
- **float→double promotion** at no-prototype call sites (same path as variadic
  float→double promotion added in stage 125).
- **char/short→int promotion** handled automatically by existing `movsx`/`movzx`
  load instructions.

### Code generation
- Single-pass walk from AST to NASM Intel-syntax assembly.
- Integer promotions, UAC, and LP64-aware conversions at every binary op.
- `movzx`/`movsx` sized loads; `div`/`idiv`; `shr`/`sar`.
- String literals in `.rodata`; struct/union member addressing and block
  struct copies (`rep movsb`); `.data`/`.bss` globals.
- Struct/union by-value marshalling; designated and compound literal init.
- **Bit-field read**: load storage unit at `byte_offset`, shift right by
  `bit_offset`, mask with `(1 << bit_width) - 1` — stage 134.
- **Bit-field write**: read-modify-write — load unit, clear bits with mask,
  OR in shifted new value, store — stage 134.
- **Full SysV AMD64 callee-saved register ABI**: `rbx`, `r12`, `r13`, `r14`,
  `r15` saved in every function prologue and restored in all epilogue paths
  (stages 122, 127).
- **FP constant pool** (`Lfc<N>`) in `.rodata`; sign-mask literals for
  `xorps`/`xorpd` in `.rodata` with 16-byte alignment.
- Per-file codegen teardown; dynamic Vec/StrBuf storage (no fixed caps).

### Tooling & diagnostics
- `--version`, `--print-ast`, `--print-tokens`, `--print-asm` modes.
- `--max-errors=N` multiple-error collection with parser recovery.
- Multi-mode build workflow (`build.sh`): `--mode=normal` / `bootstrap` /
  `fallback` / `self-host` (C0→C1→C2 with test runs and checkpoint commits).
- Strict ISO C99 source; CMake build with `-Wall -Wextra -pedantic`.

## Stage-by-Stage Timeline (100–138)

Stages 01–65 are catalogued in `project-status-through-stage-65.md`,
66–75 in `project-status-through-stage-75-06.md`, and 76–99 in
`project-status-through-stage-99.md`; new stages since:

| Stage      | Focus                                                                  |
|------------|------------------------------------------------------------------------|
| 100        | File-scope integer constant expressions; `sizeof` in eval_const_expr   |
| 101        | Block-scope static arrays, structs, and unions                         |
| 102        | Static aggregate designated initializers, struct/union elements, 2D    |
| 103        | Block-scope static scalar constant-expression initializers (`eval_const_init`) |
| 104        | Complete constant-expression evaluator (relational, equality, logical, ternary) |
| 105        | `#pragma` / `#pragma once` / `_Pragma` / `#line` / null directive     |
| 106        | `restrict` qualifier; stub header completion; `long long` return fix   |
| 107        | `inline` keyword; functional `va_copy` codegen; `<assert.h>` stub     |
| 108        | `#elifdef` / `#elifndef` preprocessor directives                       |
| 109        | `float`/`double` types, literals, stack/global variables               |
| 110        | FP arithmetic, UAC, casts (+ - * /; int↔FP conversions)               |
| 111        | FP comparisons and boolean contexts (if/while/ternary/logical NOT)     |
| 112        | FP calling convention (XMM regs); `va_arg` for `double`; `<math.h>`   |
| 113        | Test suite reorganization (21 category subfolders)                     |
| 114        | Legacy test integration; 7 FP array/struct/ternary codegen bug fixes   |
| 115        | Constant expressions in array dimension bounds                         |
| 116        | Global struct array BSS fix; `char[N]` string-literal initialization   |
| 117        | FP struct member rvalue read bug fixes (dot/arrow/subscript-then-dot)  |
| 118        | Pointer relational operators (`<` `<=` `>` `>=`) — unsigned setcc      |
| 119        | FP compound assignment on global struct members (6 bug fixes)          |
| 120        | FP `++`/`--` on struct members (SSE2 path in `codegen_inc_dec_general`)|
| 121        | `switch` on `long`/`long long`/`unsigned long long` discriminants      |
| 122        | Callee-saved `rbx` preservation per SysV AMD64 ABI                    |
| 123        | ABI fixes: FP stack args, indirect FP calls, address-constant inits    |
| 124        | Octal literals; `__func__` predefined identifier; file-scope CL ptrs  |
| 125        | FP globals from int initializers; variadic float→double promotion      |
| 126        | Tentative definitions (C99 §6.9.2)                                     |
| 127        | Callee-saved r12–r15; full SysV AMD64 GP callee-save ABI               |
| 128        | FP constant expressions at file scope (`eval_fp_const_expr`)           |
| 129        | Block-scope function declarations; extern incomplete arrays            |
| 130        | `va_arg` for struct/union by value (SysV AMD64 ABI classification)     |
| 131        | `sizeof` produces unsigned `size_t` result (C99 §6.5.3.4)             |
| 132        | Pointer equality with non-null integer constants (GCC/Clang extension) |
| 133        | Default argument promotions for no-prototype calls (C99 §6.5.2.2p6)   |
| 134        | Bit-field members (CC99-006) and flexible array members (CC99-007)     |
| 135        | Type compatibility fixes: pointer-to-array decl; parameter adjustment  |
| 136        | `sizeof` of pointer-arithmetic expressions (codegen bugfix)            |
| 137        | Functions returning function pointers (`(*f())(int)`) — CC99-010       |
| **138**    | **`auto` and `register` storage-class specifiers — CC99-011/012**      |

## Recently Shipped (Stages 135–138)

**Stage 135 — Type Compatibility and Pointer-to-Array Declarators.**

Fixes two C99 type compatibility bugs (CC99-008, CC99-009). `parse_parameter_declaration`
now applies array-to-pointer (C99 §6.7.5.3p7) and function-to-pointer-to-function
(C99 §6.7.5.3p8) adjustments for named declarators before compatibility checks are
performed, so `int values[3]`, `int values[]`, and `int *values` all form compatible
function parameter types and can appear in compatible redeclarations of the same
function. `parse_declarator` is extended to parse `(*name)[N]` and `(*name)[]`
pointer-to-array declarators instead of rejecting them; `ParsedDeclarator` gains
`is_ptr_to_array`, `ptr_to_array_length`, and `ptr_to_array_has_size` fields.
Indexed access `(*row)[i]` works via the existing deref + subscript codegen path.
Fixed a pre-existing bug where `(void)` in function-pointer parameter lists was
counted as one void parameter rather than zero. 6 new tests (5 valid, 1 moved from
invalid to valid); 1951 total tests pass. Self-host C0→C1→C2 verified.

**Stage 136 — sizeof of Pointer-Arithmetic Expressions.**

Fixes a bug in `sizeof_type_of_expr` in `src/codegen.c` where `sizeof` applied to
pointer-arithmetic expressions returned the element size instead of the pointer size.
Two targeted codegen fixes: (1) added a pointer/array guard in the `AST_BINARY_OP`
case for `+`/`-` operators, returning `TYPE_POINTER` (8 bytes on LP64) when either
operand is a pointer or array type; (2) added an `AST_STRING_LITERAL` case returning
`TYPE_POINTER` so string literals in binary expressions are treated as pointers.
10 new tests (8 new pointer-arithmetic sizeof cases, 2 regression guards); 1961
total tests pass. Self-host C0→C1→C2 verified.

**Stage 137 — Functions Returning Function Pointers.**

Ships the `int (*get_adder())(int)` declarator form (CC99-010). `parse_declarator`
now fully parses `(*name())(params)` instead of rejecting it; `ParsedDeclarator`
gains `is_func_returning_fp`, `own_param_types`, `own_param_count`, and
`own_is_no_prototype` fields for the outer parameter list. In
`parse_external_declaration`, an `is_func_returning_fp` branch constructs the nested
pointer-to-function return type, creates `AST_FUNCTION_DECL` with
`decl_type = TYPE_POINTER`, registers the function in the function table, and
optionally parses its body. A guard on `inner_stars == 0` remains to reject the
invalid direct-function-return form `int (f())(int)`. Fixed a pre-existing bug where
typedef'd pointer return types were not recognized via the `func->full_type`
assignment condition. No codegen changes needed — function pointers and indirect
calls already worked. 4 new valid tests, 1 new invalid test (1 invalid test moved to
valid since functions returning function pointers are now valid); 1965 total tests
pass. Self-host C0→C1→C2 verified.

**Stage 138 — auto and register Storage-Class Specifiers.**

Ships `auto` and `register` storage-class specifiers (CC99-011, CC99-012). New
tokens `TOKEN_AUTO` and `TOKEN_REGISTER` added with keyword recognition and display
names. `parse_declaration_specifiers` recognizes both but rejects them at file scope.
`parse_statement` handles `auto` (tagged `SC_AUTO = 8`, generates identical code to
plain automatic storage) and `register` (tagged `SC_REGISTER = 16`) at block scope.
`parse_parameter_declaration` accepts `register` as a leading qualifier with no
semantic effect on the parameter type. `SC_AUTO` and `SC_REGISTER` added to the
`StorageClass` enum in `include/ast.h`; both displayed by `ast_pretty_printer`.
`LocalVar` gains `is_register`; `AST_ADDR_OF` rejects address-of on register
variables at compile time. 5 new tests (2 valid returning 27, 3 invalid); 1970
total tests pass. Self-host C0→C1→C2 verified.

## Out of Scope (Not Yet Implemented)

- `volatile` code-generation semantics (currently parsed and tracked only);
  `restrict` qualifier (parsed and discarded).
- `va_copy` codegen complete (functional 24-byte copy implemented stage 107,
  but `va_copy` of struct-containing `va_list` not exercised).
- Multi-character constants (`'ab'`); wide-character literals.
- Chained designators (`.a.b`, `.arr[0]`); designated union init for
  non-first members.
- Non-pointer compound literals at file scope.
- Old-style (K&R) function definitions; `__attribute__` specifiers.
- `#pragma` (parsing complete; beyond `#pragma once` ignored); GNU variadic
  macro extensions (`__VA_OPT__`, named variadic args, comma deletion).
- Object-file (`.o`) emission and separate linking; header-only precompilation.

## Architecture

```
src/
├── preprocessor.c       Two-phase preprocessor (splicing, comments, directives, macros)
├── lexer.c              Tokenizer (float/double; octal; hex literals; hex/octal escapes; auto/register)
├── parser.c             Recursive-descent parser, builds AST; setjmp/longjmp recovery
├── ast.c                AST node lifecycle helpers (dynamic children array; ast_clone)
├── ast_pretty_printer.c --print-ast renderer
├── type.c               Type system (singletons + heap pointer chains; FP types)
├── codegen.c            Single-pass walker → NASM Intel-syntax asm; SysV ABI; FP/XMM
├── compiler.c           Driver (multi-file loop; compile_one_file; per-file teardown)
├── version.c            Build/version identifier (VERSION_STAGE = "13800000")
├── vec.c                Generic growable-array container (stage 95-02)
├── strbuf.c             Dynamic character/string buffer (stage 95-03)
└── util.c               Misc helpers; compile_error_at / compile_warning_at; reset_error_state
```

The grammar is documented in `docs/grammar.md`, the parser call hierarchy
in `docs/other/stage-138-parse-tree.md`, and the feature checklist in
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
