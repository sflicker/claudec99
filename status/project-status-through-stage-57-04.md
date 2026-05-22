# ClaudeC99 Project Status — Through Stage 57-04

_As of 2026-05-21 (commit `998671d`)_

## Overview

ClaudeC99 is a from-scratch C99 subset compiler written in C, targeting
x86_64 Linux via NASM (Intel syntax). The compiler is built incrementally
through small, spec-driven stages — each stage is a self-contained
specification followed by a kickoff, implementation, and milestone /
transcript record. Output is single-file assembly that is assembled with
`nasm -f elf64` and linked with `gcc -no-pie` (so crt0 / libc are
available for declared libc calls such as `puts` and `printf`).

**Stages completed: 133** (stage-01 through stage-57-04).

## Build & Test

| Component                     | Count          |
|-------------------------------|----------------|
| Source files (`src/*.c`)      | 9              |
| Header files (`include/`)     | 9              |
| Total LOC (src + include)     | 10,207         |
| Valid runtime tests           | 638            |
| Invalid (compile-error) tests | 202            |
| Integration tests             | 31             |
| Print-AST golden tests        | 39             |
| Print-tokens golden tests     | 99             |
| Print-asm golden tests        | 21             |
| **Total tests**               | **1,030**      |
| Git commits                   | 426            |

All 1,030 tests pass with no regressions.

## Language Features Implemented

### Preprocessing

The compiler now ships a full preprocessing pipeline that runs before
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
- **Predefined macros**: `__FILE__` (string literal of source path),
  `__LINE__` (current decimal line number), `__STDC__` (1),
  `__STDC_VERSION__` (199901), `__CLAUDEC99__` (1).
- **Command-line flags**: `-DNAME` / `-DNAME=VALUE` pre-defines macros;
  `-I<dir>` / `-I <dir>` adds include search paths.

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
- `while`, `do … while`, `for` (with optional init/cond/update; initializer
  is an expression only — declaration form not yet supported).
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
- Integer literal `U` / `u` suffix marks the literal as unsigned.
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
- Abstract pointer parameters in prototypes: `int puts(char *)` (unnamed
  pointer parameter accepted in function declarations and function pointer
  signatures).
- Comma-separated init-declarator lists sharing a base type:
  `int a, *b, c = 3;` (array declarators not permitted in lists).
- Brace-enclosed initializer lists for local arrays and structs (partial
  initialization zero-fills remaining elements/members); array size inferred
  from brace-list element count when explicit size is omitted.
- File-scope array declarations with brace-list or string-literal
  initializers; size inferred from list element count or string length+1
  when explicit size is omitted; too many initializers for an explicit
  size is a compile-time error.
- Local variables with optional initializers per declarator.
- Local `char` arrays initialized from a string literal, with
  explicit (`char s[5] = "abcd";`) or inferred (`char s[] = "abcd";`)
  size.
- File-scope (global) variable declarations: uninitialized (`.bss`,
  zero-initialized) and initialized with constant integer expressions
  (`.data`); string-literal pointer globals; aggregate struct initializers
  at file scope; RIP-relative addressing.
- Storage-class specifiers `extern` and `static` at file scope and
  function scope; linkage consistency enforced; `extern` with initializer
  rejected.
- `typedef` aliases for scalar types, pointer types, array types, function
  pointer types, and complete struct types; block-scoped typedef shadowing;
  unsigned scalar typedefs preserve signedness.
- Struct definitions with natural-alignment field layout, member access via
  `.` (dot) and `->` (arrow) in both rvalue and lvalue contexts, whole-struct
  assignment and copy-initialization, pointer-to-struct parameters with
  field mutation, nested struct members, arrays of structs, and typedef
  aliases for struct types.
- Incomplete struct forward declarations (opaque pointers, self-referential
  linked-list nodes); placeholder mutated in-place when body arrives.
- Enum declarations with auto-incrementing constants and explicit values;
  enumerator constants fold to `AST_INT_LITERAL` at parse time.
- Function parameters (typed) and typed return values, including pointer
  and struct-pointer return types.
- Forward declarations and definitions; signature consistency enforced
  (return type and parameter types checked at registration time).
- `NULL` as a null pointer constant (integer 0 in pointer context).
- Declaration specifier parsing accepts a list of specifiers in any order,
  detecting duplicates with clear error messages; `const` type qualifier
  accepted alongside storage class and type specifier.

### Expressions
- Integer literals with optional `L` / `l` suffix and overflow-aware
  literal typing; optional `U` / `u` suffix for unsigned literals.
- String literals with full simple-escape decoding
  (`\n`, `\t`, `\r`, `\\`, `\"`, `\0`); decay to `char *` in
  expressions.
- Character literals, including the full simple-escape set
  (`\a`, `\b`, `\f`, `\n`, `\r`, `\t`, `\v`, `\\`, `\'`, `\"`, `\?`,
  `\0`); evaluated as `int` and usable in any integer context.
- Variable reference.
- All eleven assignment operators: `=`, `+=`, `-=`, `*=`, `/=`, `%=`,
  `<<=`, `>>=`, `&=`, `^=`, `|=`; chained assignment.
- Comma operator (lowest precedence, left-associative, result type =
  right operand; distinct from argument-list and declarator-list commas).
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
- `sizeof(type)` and `sizeof expr` — operand is not evaluated; result
  is a compile-time constant of type `long`; arrays yield total size
  (element_size × count) without decay; struct types yield padded size;
  `sizeof(void)` rejected.
- Address-of `&` and dereference `*`; dereference as both rvalue and
  lvalue (`*p = …`).
- Array subscript `a[i]` and pointer subscript `p[i]` (equivalent to
  `*(p + i)`); multi-level subscript `a[i][j]` supported;
  array-to-pointer decay where required.
- Member access `.` and arrow `->` as both rvalue and lvalue; chained
  member access (e.g. `r.origin.x`, `n.next->value`).
- Function calls with up to 6 arguments (SysV register passing);
  argument-to-parameter conversion (widen / narrow / sign-extend) at
  the call boundary; return-value conversion at the function exit.
- Variadic function calls: at least the declared fixed argument count
  required; no upper-bound check on extra arguments.
- Indirect function calls through function pointers: `fp(args)` and
  `(*fp)(args)`.
- `void *` implicit conversion to/from any non-function pointer type.

### Code generation
- Single-pass walk from AST to NASM Intel-syntax assembly.
- Per-function stack frames; locals laid out with natural alignment
  for their declared sizes (1 / 2 / 4 / 8 bytes); struct locals
  zero-filled then initialized field-by-field.
- Integer promotions and common-type rules between operands of
  arithmetic / comparison / bitwise operators; Usual Arithmetic
  Conversions for mixed signed/unsigned operands.
- Unsigned loads use `movzx` (zero-extend); signed loads use `movsx`
  (sign-extend); unsigned division uses `div`; logical right shift uses
  `shr`; unsigned comparisons use `setb`/`seta`/`setbe`/`setae`.
- Pointer values stored / loaded as full 8-byte slots; pointer-store
  width selected by the pointed-to base type.
- String literals emitted as labeled byte sequences in `.rodata`;
  references decay to `char *` at use sites.
- Shift count loaded into `cl`; `sar` for arithmetic (signed) right
  shift, `shr` for logical (unsigned) right shift.
- Remainder via `idiv` / `idivq` (signed) or `div` (unsigned); result
  extracted from `rdx` / `edx`.
- Global variable sections: `.data` for initialized globals (with
  `dq label` for function-pointer label initializers), `.bss` for
  uninitialized; RIP-relative load/store for all globals.
- Non-static functions emit `global` NASM directive; static functions
  omit it.
- Void functions emit implicit epilogue on fall-off-end.
- Struct member addresses computed with field offsets; struct copies
  done byte-by-byte.
- Indirect calls use `call r10`.
- Variadic calls precede the `call` instruction with `xor eax, eax`
  (SysV AMD64: AL = number of floating-point arguments in XMM registers;
  zero because this compiler does not support floating-point).

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
| **57-04**  | **Variadic macros (`__VA_ARGS__`) (current)**                 |

## Recently Shipped (Stages 48–57-04)

**Stage 48** introduced the preprocessor module (`src/preprocessor.c`,
`include/preprocessor.h`): two-phase processing strips comments and joins
backslash-newline continuations before the lexer sees the source.

**Stages 49–51** added the macro system: `#include "file.h"` with
directory-relative resolution, object-like `#define`, and function-like
`#define` with parameter substitution and recursive expansion.

**Stages 52-01 through 52-04** added conditional compilation:
`#ifdef` / `#ifndef` / `#if` / `#elif` / `#else` / `#endif` with
nesting support (depth 64) and first-true-wins `#elif` chains.

**Stage 53** added `__FILE__` and `__LINE__` predefined macros.
**Stage 54** added `#undef`.

**Stages 55-01 through 55-09** built out a full recursive-descent
expression evaluator for `#if` / `#elif` conditions: `defined()`,
macro expansion, undefined-identifiers-to-0, unary operators,
parentheses, comparisons, logical operators, arithmetic, and bitwise /
shift operators.

**Stages 56-01 through 56-05** added `#error`, command-line `-D` and
`-I` flags, angle-bracket `#include <file.h>`, and standard predefined
macros (`__STDC__`, `__STDC_VERSION__`, `__CLAUDEC99__`).

**Stage 57-01** added stringification (`#param` in function-like macros).
**Stage 57-02** added token pasting (`##`).
**Stage 57-03** added variadic function declarations and calls; the
`...` ellipsis is parsed in parameter lists; call sites accept at least
the fixed parameter count.
**Stage 57-04** added variadic macros: `#define M(...)` and
`#define M(x, ...)` with `__VA_ARGS__` substitution.

## Out of Scope (Not Yet Implemented)

- Floating-point types (`float`, `double`).
- `for`-loop initializer declarations (`for (int i = 0; …)`).
- Variadic function *definitions* (`va_list`, `va_start`, `va_arg`,
  `va_end`, `<stdarg.h>`); only declarations and external calls are
  supported.
- Octal (`'\123'`) and hex (`'\x41'`) character escapes.
- Multi-character constants (`'ab'`); wide-character literals.
- Calls with more than 6 arguments (calling-convention limit).
- Multi-dimensional arrays beyond 2-D (only `a[i][j]` is currently
  exercised via the single relaxed postfix subscript base rule).
- Unions, bit-fields, flexible array members, compound literals.
- Pointer-level `const` enforcement (writes through `const *`,
  conversion const-correctness).
- `typedef enum`.
- Enum constant expressions beyond integer/character literals.
- Functions returning function pointers.
- `#pragma` (including `#pragma once`).
- `__DATE__` and `__TIME__` predefined macros.
- GNU extension variadic macro features (`__VA_OPT__`, named variadic
  args, comma deletion).

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
├── compiler.c           Driver (command-line parsing, -D/-I flags)
└── util.c               Misc helpers
```

The grammar is documented in `docs/grammar.md` and is updated
alongside any stage that touches it. A full parse-tree diagram is in
`docs/other/stage-57-04-parse-tree.md`.

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
