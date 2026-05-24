# ClaudeC99 Project Status — Through Stage 65

_As of 2026-05-24 (commit `2c51317`)_

## Overview

ClaudeC99 is a from-scratch C99 subset compiler written in C, targeting
x86_64 Linux via NASM (Intel syntax). The compiler is built incrementally
through small, spec-driven stages — each stage is a self-contained
specification followed by a kickoff, implementation, and milestone /
transcript record. Output is single-file assembly that is assembled with
`nasm -f elf64` and linked with `gcc -no-pie` (so crt0 / libc are
available for declared libc calls such as `puts` and `printf`).

**Stages completed: 142** (stage-01 through stage-65).

## Build & Test

| Component                     | Count          |
|-------------------------------|----------------|
| Source files (`src/*.c`)      | 9              |
| Header files (`include/`)     | 9              |
| Total LOC (src + include)     | 10,500         |
| Valid runtime tests           | 694            |
| Invalid (compile-error) tests | 211            |
| Integration tests             | 53             |
| Print-AST golden tests        | 39             |
| Print-tokens golden tests     | 99             |
| Print-asm golden tests        | 21             |
| **Total tests**               | **1,117**      |
| Git commits                   | 460            |

All 1,117 tests pass with no regressions.

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
- **Stringification**: `#param` in function-like replacement lists
  converts an argument to a string literal with whitespace normalization
  and escape handling.
- **Token pasting**: `##` concatenates adjacent tokens in replacement
  lists; `##` at start or end is a compile-time error.
- **`#undef NAME`**: removes a macro from the macro table; no-op if
  undefined.
- **Conditional compilation**: `#ifdef` / `#ifndef` / `#if` / `#elif` /
  `#else` / `#endif`; nesting up to 64 levels; first-true-wins
  `#elif` chains.
- **Preprocessor conditional expressions**: full expression evaluator
  for `#if` / `#elif` supporting integer literals, `defined(NAME)`,
  object-like macro expansion, undefined identifiers evaluate to 0,
  unary `!` / `-` / `+` / `~`, parentheses, `==` / `!=` / `<` / `<=` /
  `>` / `>=`, `&&` / `||`, `+` / `-` / `*` / `/` / `%`, `&` / `^` /
  `|`, `<<` / `>>`.
- **`#error message`**: halts compilation with user-supplied message;
  skipped in inactive regions.
- **Predefined macros — standard**: `__FILE__` (string literal of
  source path), `__LINE__` (current decimal line number), `__DATE__`
  (`"Mmm DD YYYY"`), `__TIME__` (`"HH:MM:SS"`), `__STDC__` (1),
  `__STDC_VERSION__` (199901), `__STDC_HOSTED__` (1),
  `__CLAUDEC99__` (1).
- **Predefined macros — ABI/target**: `__x86_64__`, `__linux__`,
  `__unix__`, `__LP64__`, `_LP64`, `__CHAR_BIT__` (8), and the full
  `__SIZEOF_*__` family (`CHAR`=1, `SHORT`=2, `INT`=4, `LONG`=8,
  `POINTER`=8, `SIZE_T`=8, `LONG_LONG`=8); injected unconditionally
  before user source.
- **Command-line flags**: `-DNAME` / `-DNAME=VALUE` pre-defines macros;
  `-I<dir>` / `-I <dir>` adds include search paths; `--sysroot=<dir>`
  rewrites absolute `-I` paths.
- **Stub system headers** in `test/include/`: `stdio.h`, `stddef.h`,
  `stdlib.h`, `string.h`, `limits.h` (including `LLONG_MIN`,
  `LLONG_MAX`, `ULLONG_MAX`), `stdint.h` (exact-width
  `int8_t`…`uint64_t`, pointer-size, fast, and least variants),
  `stdbool.h` (`bool`, `true`, `false`).

### Core program shape
- Translation unit with one or more external declarations.
- Function definitions and forward declarations (with return-type and
  parameter-type compatibility checking across redeclarations).
- Multiple functions per translation unit; recursive calls.
- Variadic function declarations and external calls (e.g. `printf`);
  `is_variadic` flag on function signatures; call sites require at least
  the fixed parameter count; `xor eax, eax` emitted before variadic
  calls per SysV AMD64 convention.
- `main` is the entry point; non-`main` functions return via SysV
  AMD64 calling convention.
- Calls into libc (e.g. `int puts(char *s);`, `int printf(const char *, ...);`)
  emitted via NASM `extern` and resolved by linking with `gcc -no-pie`.

### Statements
- `return <expr>;` and bare `return;` (for void functions).
- `if` / `else` (chained, with blocks).
- `while`, `do … while`, `for` (with optional init/cond/update).
- `switch` / `case` / `default` (linear dispatch; nested switches OK).
- `break`, `continue`, `goto` + labeled statement.
- Compound block statements with proper lexical scoping (shadowing
  permitted; duplicate declaration in the same scope rejected).
- Expression statements.

### Declarations & types
- Integer base types: `char`, `short`, `int`, `long` (signed).
- Unsigned integer types: `unsigned char`, `unsigned short`,
  `unsigned int`, `unsigned long`, plain `unsigned` (= unsigned int);
  unsigned arithmetic, division, right shift, and comparisons use
  appropriate unsigned instructions.
- `signed` keyword: `signed char`, `signed short`, `signed int`,
  `signed long`, plain `signed` (= int); trailing `int` forms accepted
  (`short int`, `long int`, `unsigned short int`, etc.).
- `long long` and `unsigned long long` (8 bytes, alignment 8; codegen
  identical to `long`); `LL`/`ll` and `ULL`/`LLU` literal suffixes.
- `_Bool` type with value-normalization semantics: any nonzero value
  assigned to `_Bool` becomes 1; zero stays 0; integer promotion applies
  in expressions (promotes to `int`).
- Integer literal suffixes: `U`/`u` (unsigned), `L`/`l` (long),
  `UL`/`LU` (unsigned long), `LL`/`ll` (long long), `ULL`/`LLU`
  (unsigned long long); overflow-aware literal typing.
- `void` type for void functions, void parameters, and generic `void *`
  pointers; dereferencing, arithmetic, and subscripting void pointers
  rejected.
- `const` qualifier on base-type scalars; assignment to const-qualified
  lvalues rejected.
- Pointer types of arbitrary depth: `int *`, `int **`, `char *`, etc.
- Array types: `T xs[N];` with single-bracket suffix; multi-level
  subscript `a[i][j]` supported.
- Function pointer types: `int (*fp)(int, char)` in local, parameter,
  file-scope, static, and extern positions; indirect calls via `fp(args)`
  and `(*fp)(args)`.
- Parenthesized declarators: `int (*p)` as grouping equivalent to `int *p`.
- Abstract pointer parameters in prototypes: `int puts(char *)`.
- Comma-separated init-declarator lists sharing a base type:
  `int a, *b, c = 3;`.
- Brace-enclosed initializer lists for local arrays and structs (partial
  initialization zero-fills remaining elements/members); array size inferred
  from brace-list element count when explicit size is omitted.
- File-scope array declarations with brace-list or string-literal
  initializers; size inferred from list element count or string length+1.
- Local variables with optional initializers per declarator.
- Local `char` arrays initialized from a string literal, with
  explicit or inferred size.
- File-scope (global) variable declarations: uninitialized (`.bss`,
  zero-initialized) and initialized with constant integer expressions
  (`.data`); string-literal pointer globals; aggregate struct initializers
  at file scope; RIP-relative addressing.
- Storage-class specifiers `extern` and `static` at file scope and
  function scope; linkage consistency enforced.
- `typedef` aliases for scalar types, pointer types, array types, function
  pointer types, and complete struct types; block-scoped typedef shadowing.
- Struct definitions with natural-alignment field layout, member access via
  `.` and `->` in both rvalue and lvalue contexts, whole-struct assignment
  and copy-initialization, pointer-to-struct parameters with field mutation,
  nested struct members, arrays of structs, and typedef aliases.
- Incomplete struct forward declarations (opaque pointers, self-referential
  linked-list nodes).
- Enum declarations with auto-incrementing constants and explicit values;
  enumerator constants fold to `AST_INT_LITERAL` at parse time.
- `NULL` as a null pointer constant.

### Expressions
- Integer literals with optional suffixes and overflow-aware typing.
- String literals with full simple-escape decoding; decay to `char *`.
- Character literals, including the full simple-escape set; evaluated as
  `int`.
- Variable reference.
- All eleven assignment operators: `=`, `+=`, `-=`, `*=`, `/=`, `%=`,
  `<<=`, `>>=`, `&=`, `^=`, `|=`; chained assignment.
- Comma operator (lowest precedence, left-associative).
- Conditional (ternary) operator `cond ? t : f` with common-type
  branches and short-circuit evaluation.
- Arithmetic: `+`, `-`, `*`, `/`, `%` (signed and unsigned integer).
- Bitwise: `&`, `^`, `|`, `~` (complement), `<<`, `>>` (arithmetic
  right shift for signed; logical right shift for unsigned).
- Pointer arithmetic: `p + n`, `p - n`, `p1 - p2`, scaled by element
  size; pointer difference yields `long`.
- Equality / relational: `==`, `!=`, `<`, `<=`, `>`, `>=`.
- Logical: `&&`, `||`, `!` (with short-circuit evaluation).
- Prefix and postfix `++` / `--`.
- Casts: `(<integer-type>) <expr>`; accepts `void`, scalar, unsigned,
  and typedef types.
- Integer promotions and Usual Arithmetic Conversions for mixed
  signed/unsigned operands; LP64-specific rule (signed `long` vs
  `unsigned int`: unsigned converts to long since long can represent
  all unsigned int values).
- `sizeof(type)` and `sizeof expr` — operand is not evaluated; result
  is a compile-time constant of type `long`; arrays yield total size
  without decay; struct types yield padded size; `sizeof(void)` rejected.
- Address-of `&` and dereference `*`; dereference as both rvalue and
  lvalue (`*p = …`).
- Array subscript `a[i]` and pointer subscript `p[i]`; multi-level
  subscript `a[i][j]` supported; array-to-pointer decay.
- Member access `.` and arrow `->` as both rvalue and lvalue; chained
  member access.
- Function calls with up to 6 arguments (SysV register passing);
  argument-to-parameter conversion at the call boundary.
- Variadic function calls: at least the declared fixed argument count
  required.
- Indirect function calls through function pointers: `fp(args)` and
  `(*fp)(args)`.
- `void *` implicit conversion to/from any non-function pointer type.

### Code generation
- Single-pass walk from AST to NASM Intel-syntax assembly.
- Per-function stack frames; locals laid out with natural alignment.
- Integer promotions, UAC, and LP64-aware signed/unsigned conversion
  rules applied at every binary operation and comparison.
- Unsigned loads use `movzx`; signed loads use `movsx`; unsigned
  division uses `div`; logical right shift uses `shr`; unsigned
  comparisons use `setb`/`seta`/`setbe`/`setae`.
- `_Bool` assignment normalizes through a zero-compare: store 1 if the
  value is nonzero, 0 if zero.
- `long long` and `unsigned long long` operations use the 64-bit
  register file identically to `long`.
- String literals emitted as labeled byte sequences in `.rodata`.
- Struct member addresses computed with field offsets; struct copies
  done byte-by-byte.
- Global variable sections: `.data` for initialized globals, `.bss` for
  uninitialized; RIP-relative load/store for all globals.
- Non-static functions emit `global` NASM directive; static functions
  omit it.
- Indirect calls use `call r10`.
- Variadic calls precede the `call` instruction with `xor eax, eax`.

## Stage-by-Stage Timeline

| Stage      | Focus                                                         |
|------------|---------------------------------------------------------------|
| 01         | Minimal compiler — `return <int_literal>;`                    |
| 02         | Integer arithmetic expressions                                |
| 03         | Equality / relational expressions                             |
| 04         | Unary operators, `if` / `else`, block statements             |
| 05         | Integer variables with initializers                           |
| 06         | `while` statement                                             |
| 07-01      | Assignment expressions                                        |
| 07-02      | `for` statement                                               |
| 07-03      | Increment / decrement (`++`, `--`)                            |
| 07-04      | AST pretty-printer (`--print-ast`)                            |
| 08-01      | Logical operators (`&&`, `\|\|`) with short-circuit           |
| 08-02      | Nested lexical scopes                                         |
| 09-01      | Reorganize source into multiple files                         |
| 09-02      | Grammar updates                                               |
| 09-03      | Multiple functions per translation unit                       |
| 09-04      | Function calls                                                |
| 09-05      | Function forward declarations                                 |
| 10-01      | `break` / `continue`                                          |
| 10-02      | `do … while`                                                  |
| 10-03-01   | `switch` / `default` skeleton                                 |
| 10-03-02   | `case` and dispatch                                           |
| 10-03-03   | Standard `switch` semantics                                   |
| 10-04      | `goto` and labeled statements                                 |
| 10-05      | Line / block comments                                         |
| 11-01      | Minimal type system (`char` / `short` / `int` / `long`)       |
| 11-02      | Integer-type-aware codegen                                    |
| 11-03      | Promotions and common arithmetic type                         |
| 11-04      | Types for equality / logical / conditional                    |
| 11-05-01   | Typed parameter & return grammar                              |
| 11-05-02   | Typed function signatures and call semantics                  |
| 11-05-03   | Conversions at the function boundary                          |
| 11-06      | Casting expressions                                           |
| 11-07      | Integer-literal typing with `L` suffix and overflow           |
| 12-01      | Pointer types in declarations                                 |
| 12-02      | Address-of `&` and dereference `*` (rvalue)                   |
| 12-03      | Assignment through pointer (`*p = …`)                         |
| 12-04      | Pointer parameters and arguments                              |
| 12-05      | Pointer return types                                          |
| 12-06      | `NULL` null pointer constant                                  |
| 13-01      | Array types in declarations                                   |
| 13-02      | Array indexing                                                |
| 13-03      | Array-to-pointer decay                                        |
| 13-04      | Pointer arithmetic                                            |
| 13-05      | Pointer-subscript expressions                                 |
| 14-00      | Lexer mode and tests for string literals                      |
| 14-01      | String-literal tokens                                         |
| 14-02      | String-literal AST node                                       |
| 14-03      | Emit string literals to static data                           |
| 14-04      | String literals as `char *` expressions                       |
| 14-05      | String-literal escape sequences                               |
| 14-06      | `char` array initialization from string literal               |
| 14-07      | `puts` / libc emission                                        |
| 14-07-01   | libc test adjustment                                          |
| 14-08      | Additional invalid tests for char-array support               |
| 15-01      | Character-literal lexer support                               |
| 15-02      | Character-literal AST + codegen                               |
| 15-03      | Character-literal type rules and integration                  |
| 15-04      | Character-literal escape and diagnostic completion            |
| 16-01      | Remainder operator (`%`)                                      |
| 16-02      | Unary bitwise complement (`~`)                                |
| 16-03      | Shift operators (`<<`, `>>`)                                  |
| 16-04      | Bitwise binary operators (`&`, `^`, `\|`)                     |
| 16-05      | Remaining compound assignment operators                       |
| 17-01      | `sizeof(type)`                                                |
| 17-02      | `sizeof expr`                                                 |
| 17-03      | `sizeof` for array types                                      |
| 18         | Conditional (ternary) operator `?:`                           |
| 19         | Comma operator                                                |
| 20-01      | Declarator parsing refactoring                                |
| 20-02      | Comma-separated init-declarator lists                         |
| 20-03      | Declaration regression tests                                  |
| 21-01      | Function declarator refactoring                               |
| 21-02      | Separate function definitions from declarations               |
| 21-03      | Function declaration compatibility checks                     |
| 22-01      | File-scope uninitialized object declarations (`.bss`)         |
| 22-02      | Global initializers and file-scope declaration validation     |
| 23         | Storage-class basics (`extern`, `static`)                     |
| 24         | Parenthesized declarators                                     |
| 25-01      | Function pointer declarations                                 |
| 25-02      | Assign function designators to function pointers              |
| 25-03      | Indirect function calls through function pointers             |
| 26         | General declarator cleanup (unified declarator machinery)     |
| 27         | Declaration specifier list refactor                           |
| 28-01      | Simple typedef names (scalar type aliases)                    |
| 28-02      | Pointer typedef support                                       |
| 28-03      | Function pointer typedefs                                     |
| 28-04      | Array typedefs                                                |
| 29         | Enum declarations with compile-time constants                 |
| 30         | Struct definition, layout, and `sizeof`                       |
| 31         | Struct member access (`.` and `->`)                           |
| 32         | Aggregate initializer lists (arrays and structs)              |
| 33         | Struct assignment and copy-initialization                     |
| 34         | Struct pointer parameters and mutation                        |
| 35         | Nested structs and arrays of structs                          |
| 36         | `typedef` alias for complete struct types                     |
| 37         | Incomplete struct forward declarations                        |
| 37-02      | Additional struct tests; chained arrow access fixes           |
| 38         | `void` type and `void *` generic pointer                      |
| 39         | Minimal `const` qualifier support                             |
| 40         | Unsigned integer types and `size_t`                           |
| 00-98      | Benchmark support and `U`/`u` integer literal suffix          |
| 41         | Pointer arithmetic completion (difference, ±=, ++/--)         |
| 42         | Arrays of pointers and multi-level subscript (`a[i][j]`)      |
| 43         | File-scope array and string initializers; size inference       |
| 44         | Aggregate initializers for structs and arrays of structs      |
| 45         | libc prototypes and integration test harness                  |
| 46         | Command-line arguments (`argc` / `argv`) — verification       |
| 47         | Multi-file integration test infrastructure                    |
| 48         | Preprocessor foundation (comments, line splicing)             |
| 49         | Local quoted includes (`#include "file.h"`)                   |
| 50         | Object-like macros (`#define NAME replacement`)               |
| 51         | Function-like macros (`#define NAME(params) replacement`)     |
| 52-01      | `#ifdef` / `#ifndef` / `#else` / `#endif`                    |
| 52-02      | Nested conditional processing and include guards              |
| 52-03      | `#if <integer-constant>` conditionals                         |
| 52-04      | `#elif <integer-constant>` conditionals                       |
| 53         | Predefined macros `__FILE__` and `__LINE__`                   |
| 54         | `#undef` directive                                            |
| 55-01      | `defined()` operator in preprocessor conditionals             |
| 55-02      | Macro expansion and `defined NAME` in conditionals            |
| 55-03      | Undefined identifiers evaluate to 0 in conditionals           |
| 55-04      | Unary `!`, `-`, `+` in preprocessor conditional expressions   |
| 55-05      | Parenthesized expressions in preprocessor conditionals        |
| 55-06      | Equality and relational operators in preprocessor conditionals|
| 55-07      | Logical `&&` and `\|\|` in preprocessor conditionals          |
| 55-08      | Arithmetic operators in preprocessor conditionals             |
| 55-09      | Bitwise and shift operators in preprocessor conditionals      |
| 56-01      | `#error` directive                                            |
| 56-02      | Command-line `-D` macro definitions                           |
| 56-03      | Command-line `-I` include search paths                        |
| 56-04      | Angle-bracket includes (`#include <file.h>`)                  |
| 56-05      | Standard predefined macros (`__STDC__`, `__STDC_VERSION__`, `__CLAUDEC99__`) |
| 57-01      | Macro stringification (`#param`)                              |
| 57-02      | Token pasting (`##`)                                          |
| 57-03      | Variadic function declarations and calls (`...`)              |
| 57-04      | Variadic macros (`__VA_ARGS__`)                               |
| 58         | Controlled standard-style stub headers in `test/include/`     |
| 59         | Controlled header hardening — integration test expansion      |
| 60-01      | Static predefined ABI/target macros (`__x86_64__` etc.)       |
| 60-02      | Runtime predefined macros (`__DATE__`, `__TIME__`, `__STDC_HOSTED__`) |
| 61         | `signed` keyword and U/L suffix formalization                 |
| 62         | `limits.h` and `stdint.h` stub headers                        |
| 63         | `_Bool` type, `stdbool.h` stub, extended `limits.h`           |
| **64**     | **`long long` / `unsigned long long` with LL/ULL suffixes**   |
| **65**     | **Integer conversion and arithmetic hardening tests (current)**|

## Recently Shipped (Stages 58–65)

**Stage 58** added controlled stub headers to `test/include/`: `stdio.h`,
`stddef.h`, `stdlib.h`, and `string.h`, enabling angle-bracket includes
in the test suite without system header dependencies.

**Stage 59** hardened the header infrastructure with 15 new integration
tests exercising the preprocessor, mock standard headers, and core
compiler features together.

**Stage 60-01** injected 12 static target/ABI predefined macros
(`__x86_64__`, `__linux__`, `__unix__`, `__LP64__`, `_LP64`,
`__CHAR_BIT__`, and the `__SIZEOF_*__` family) unconditionally before
user source is processed.

**Stage 60-02** added three runtime-context predefined macros:
`__DATE__` (`"Mmm DD YYYY"`), `__TIME__` (`"HH:MM:SS"`), and
`__STDC_HOSTED__` (1).

**Stage 61** introduced the `signed` keyword (`signed char`, `signed
short`, `signed int`, `signed long`, plain `signed` → int) and
formalized trailing-`int` forms (`short int`, `long int`,
`unsigned short int`, etc.).

**Stage 62** added `limits.h` (with `LLONG_MIN`, `LLONG_MAX`,
`ULLONG_MAX`) and `stdint.h` (exact-width `int8_t`…`uint64_t` plus
pointer-size, fast, and least variants) stub headers.

**Stage 63** introduced `_Bool` with value-normalization semantics (any
nonzero value stored in `_Bool` becomes 1), integer promotion rules, and
the `stdbool.h` stub (`bool`, `true`, `false`).

**Stage 64** added `long long` and `unsigned long long` (8 bytes,
alignment 8; codegen identical to `long`) along with `LL`/`ll` and
`ULL`/`LLU` integer literal suffixes and the `__SIZEOF_LONG_LONG__`
predefined macro.

**Stage 65** added 17 tests to `test/valid/` exercising integer
promotion and Usual Arithmetic Conversion behaviors end-to-end: `_Bool`
and `stdint.h` type promotion via standard headers, signed/unsigned
comparison UAC rules (including the LP64 `long`-vs-`unsigned int`
case), `long long` dominance, assignment truncation to narrow unsigned
types, `_Bool` normalization, conditional expression type unification,
compound assignment wrapping, and unsigned multiplication wraparound.

## Out of Scope (Not Yet Implemented)

- Floating-point types (`float`, `double`).
- `for`-loop initializer declarations (`for (int i = 0; …)`).
- Variadic function *definitions* (`va_list`, `va_start`, `va_arg`,
  `va_end`, `<stdarg.h>`); only declarations and external calls are
  supported.
- Function calls with more than 6 arguments (ABI stack-passing not
  yet implemented — a prerequisite for self-hosting).
- Struct by-value parameters and return values (hidden-pointer ABI not
  yet implemented — a prerequisite for self-hosting).
- Block-scope `static` local variables (a prerequisite for
  self-hosting).
- Octal (`'\123'`) and hex (`'\x41'`) character escapes.
- Multi-character constants (`'ab'`); wide-character literals.
- Unions, bit-fields, flexible array members, compound literals.
- Pointer-level `const` enforcement (writes through `const *`,
  conversion const-correctness).
- `typedef enum`.
- Enum constant expressions beyond integer/character literals.
- Functions returning function pointers.
- `#pragma` (including `#pragma once`).
- `#elifdef` / `#elifndef`.
- GNU variadic macro extensions (`__VA_OPT__`, named variadic args,
  comma deletion).

## Architecture

```
src/
├── preprocessor.c       Two-phase preprocessor (splicing, comments, directives, macros)
├── lexer.c              Tokenizer
├── parser.c             Recursive-descent parser, builds AST
├── ast.c                AST node lifecycle helpers
├── ast_pretty_printer.c --print-ast renderer
├── type.c               Type system (singletons + heap pointer chains)
├── codegen.c            Single-pass walker → NASM Intel-syntax asm
├── compiler.c           Driver (command-line parsing, -D/-I/--sysroot flags)
└── util.c               Misc helpers
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
