# ClaudeC99 Project Status — Through Stage 20-03

_As of 2026-05-05 (commit `4f6486f`)_

## Overview

ClaudeC99 is a from-scratch C99 subset compiler written in C, targeting
x86_64 Linux via NASM (Intel syntax). The compiler is built incrementally
through small, spec-driven stages — each stage is a self-contained
specification followed by a kickoff, implementation, and milestone /
transcript record. Output is single-file assembly that is assembled with
`nasm -f elf64` and linked with `gcc -no-pie` (so crt0 / libc are
available for declared libc calls such as `puts`).

**Stages completed: 69** (stage-01 through stage-20-03).

## Build & Test

| Component                     | Count          |
|-------------------------------|----------------|
| Source files (`src/*.c`)      | 8              |
| Header files (`include/`)     | 8              |
| Total LOC (src + include)     | 4,784          |
| Valid runtime tests           | 393            |
| Invalid (compile-error) tests | 106            |
| Print-AST golden tests        | 24             |
| Print-tokens golden tests     | 88             |
| Print-asm golden tests        | 19             |
| **Total tests**               | **630**        |
| Git commits                   | 219            |

All 630 tests pass with no regressions.

## Language Features Implemented

### Core program shape
- Translation unit with one or more external declarations.
- Function definitions and forward declarations.
- Multiple functions per translation unit; recursive calls.
- `main` is the entry point; non-`main` functions return via SysV
  AMD64 calling convention.
- Calls into libc (e.g. `int puts(char *s);`) emitted via NASM
  `extern` and resolved by linking with `gcc -no-pie`.

### Statements
- `return <expr>;`
- `if` / `else` (chained, with blocks).
- `while`, `do … while`, `for` (with optional init/cond/update; initializer
  is an expression only — declaration form not yet supported).
- `switch` / `case` / `default` (linear dispatch; nested switches OK).
- `break`, `continue`, `goto` + labeled statement.
- Compound block statements with proper lexical scoping (shadowing
  permitted; duplicate declaration in the same scope rejected).
- Expression statements.
- Line `// …` and block `/* … */` comments.

### Declarations & types
- Integer base types: `char`, `short`, `int`, `long` (signed only).
- Pointer types of arbitrary depth: `int *`, `int **`, `char *`, etc.
- Array types: `T xs[N];` with single-bracket suffix.
- Comma-separated init-declarator lists sharing a base type:
  `int a, *b, c = 3;` (array declarators not permitted in lists).
- Local variables with optional initializers per declarator.
- Local `char` arrays initialized from a string literal, with
  explicit (`char s[5] = "abcd";`) or inferred (`char s[] = "abcd";`)
  size.
- Function parameters (typed) and typed return values, including
  pointer return types.
- Forward declarations and definitions; signature consistency
  enforced at registration time.
- `NULL` as a null pointer constant (integer 0 in pointer context).

### Expressions
- Integer literals with optional `L` / `l` suffix and overflow-aware
  literal typing.
- String literals with full simple-escape decoding
  (`\n`, `\t`, `\r`, `\\`, `\"`, `\0`); decay to `char *` in
  expressions.
- Character literals, including the full simple-escape set
  (`\a`, `\b`, `\f`, `\n`, `\r`, `\t`, `\v`, `\\`, `\'`, `\"`, `\?`,
  `\0`); evaluated as `int` and usable in any integer context.
  Malformed character literals are rejected with diagnostics.
- Variable reference.
- All eleven assignment operators: `=`, `+=`, `-=`, `*=`, `/=`, `%=`,
  `<<=`, `>>=`, `&=`, `^=`, `|=`; chained assignment.
- Comma operator (lowest precedence, left-associative, result type =
  right operand; distinct from argument-list and declarator-list commas).
- Conditional (ternary) operator `cond ? t : f` with common-type
  branches and short-circuit evaluation.
- Arithmetic: `+`, `-`, `*`, `/`, `%` (signed integer).
- Bitwise: `&`, `^`, `|`, `~` (complement), `<<`, `>>` (arithmetic
  right shift).
- Pointer arithmetic: `p + n`, `p - n`, scaled by element size.
- Equality / relational: `==`, `!=`, `<`, `<=`, `>`, `>=`.
- Logical: `&&`, `||`, `!` (with short-circuit evaluation).
- Prefix and postfix `++` / `--`.
- Casts: `(<integer-type>) <expr>`.
- `sizeof(type)` and `sizeof expr` — operand is not evaluated; result
  is a compile-time constant of type `long`; arrays yield total size
  (element_size × count) without decay.
- Address-of `&` and dereference `*`; dereference as both rvalue and
  lvalue (`*p = …`).
- Array subscript `a[i]` and pointer subscript `p[i]` (equivalent to
  `*(p + i)`); array-to-pointer decay where required.
- Function calls with up to 6 arguments (SysV register passing);
  argument-to-parameter conversion (widen / narrow / sign-extend) at
  the call boundary; return-value conversion at the function exit.

### Code generation
- Single-pass walk from AST to NASM Intel-syntax assembly.
- Per-function stack frames; locals laid out with natural alignment
  for their declared sizes (1 / 2 / 4 / 8 bytes).
- Integer promotions and common-type rules between operands of
  arithmetic / comparison / bitwise operators.
- Pointer values stored / loaded as full 8-byte slots; pointer-store
  width selected by the pointed-to base type.
- String literals emitted as labeled byte sequences in `.rodata`;
  references decay to `char *` at use sites.
- Shift count loaded into `cl`; `sar` for arithmetic right shift.
- Remainder via `idiv` / `idivq`; result extracted from `rdx` / `edx`.

## Stage-by-Stage Timeline

| Stage     | Focus                                                         |
|-----------|---------------------------------------------------------------|
| 01        | Minimal compiler — `return <int_literal>;`                    |
| 02        | Integer arithmetic expressions                                |
| 03        | Equality / relational expressions                             |
| 04        | Unary operators, `if` / `else`, block statements             |
| 05        | Integer variables with initializers                           |
| 06        | `while` statement                                             |
| 07-01     | Assignment expressions                                        |
| 07-02     | `for` statement                                               |
| 07-03     | Increment / decrement (`++`, `--`)                            |
| 07-04     | AST pretty-printer (`--print-ast`)                            |
| 08-01     | Logical operators (`&&`, `\|\|`) with short-circuit           |
| 08-02     | Nested lexical scopes                                         |
| 09-01     | Reorganize source into multiple files                         |
| 09-02     | Grammar updates                                               |
| 09-03     | Multiple functions per translation unit                       |
| 09-04     | Function calls                                                |
| 09-05     | Function forward declarations                                 |
| 10-01     | `break` / `continue`                                          |
| 10-02     | `do … while`                                                  |
| 10-03-01  | `switch` / `default` skeleton                                 |
| 10-03-02  | `case` and dispatch                                           |
| 10-03-03  | Standard `switch` semantics                                   |
| 10-04     | `goto` and labeled statements                                 |
| 10-05     | Line / block comments                                         |
| 11-01     | Minimal type system (`char` / `short` / `int` / `long`)       |
| 11-02     | Integer-type-aware codegen                                    |
| 11-03     | Promotions and common arithmetic type                         |
| 11-04     | Types for equality / logical / conditional                    |
| 11-05-01  | Typed parameter & return grammar                              |
| 11-05-02  | Typed function signatures and call semantics                  |
| 11-05-03  | Conversions at the function boundary                          |
| 11-06     | Casting expressions                                           |
| 11-07     | Integer-literal typing with `L` suffix and overflow           |
| 12-01     | Pointer types in declarations                                 |
| 12-02     | Address-of `&` and dereference `*` (rvalue)                   |
| 12-03     | Assignment through pointer (`*p = …`)                         |
| 12-04     | Pointer parameters and arguments                              |
| 12-05     | Pointer return types                                          |
| 12-06     | `NULL` null pointer constant                                  |
| 13-01     | Array types in declarations                                   |
| 13-02     | Array indexing                                                |
| 13-03     | Array-to-pointer decay                                        |
| 13-04     | Pointer arithmetic                                            |
| 13-05     | Pointer-subscript expressions                                 |
| 14-00     | Lexer mode and tests for string literals                      |
| 14-01     | String-literal tokens                                         |
| 14-02     | String-literal AST node                                       |
| 14-03     | Emit string literals to static data                           |
| 14-04     | String literals as `char *` expressions                       |
| 14-05     | String-literal escape sequences                               |
| 14-06     | `char` array initialization from string literal               |
| 14-07     | `puts` / libc emission                                        |
| 14-07-01  | libc test adjustment                                          |
| 14-08     | Additional invalid tests for char-array support               |
| 15-01     | Character-literal lexer support                               |
| 15-02     | Character-literal AST + codegen                               |
| 15-03     | Character-literal type rules and integration                  |
| 15-04     | Character-literal escape and diagnostic completion            |
| 16-01     | Remainder operator (`%`)                                      |
| 16-02     | Unary bitwise complement (`~`)                                |
| 16-03     | Shift operators (`<<`, `>>`)                                  |
| 16-04     | Bitwise binary operators (`&`, `^`, `\|`)                     |
| 16-05     | Remaining compound assignment operators                       |
| 17-01     | `sizeof(type)`                                                |
| 17-02     | `sizeof expr`                                                 |
| 17-03     | `sizeof` for array types                                      |
| 18        | Conditional (ternary) operator `?:`                           |
| 19        | Comma operator                                                |
| 20-01     | Declarator parsing refactoring                                |
| 20-02     | Comma-separated init-declarator lists                         |
| **20-03** | **Declaration regression tests (current)**                    |

## Recently Shipped (Stages 16–20)

**Stage 16** completed the bitwise operator set: `%` (remainder via
`idiv`/`idivq`), `~` (unary complement), `<<` / `>>` (arithmetic shift
with count in `cl`), and the three bitwise binary operators `&`, `^`,
`|` at their correct precedence levels. The five remaining compound
assignment operators (`*=`, `/=`, `%=`, `<<=`, `>>=`, `&=`, `^=`, `|=`)
were also added, completing all eleven assignment forms.

**Stage 17** added `sizeof` in all three syntactic forms: `sizeof(type)`,
`sizeof expr`, and `sizeof` applied to an array identifier (yielding
element_size × count without decay). In all cases the operand is not
evaluated and the result is a compile-time constant.

**Stage 18** added the conditional (ternary) operator `cond ? t : f`.
The true branch re-enters the full expression level so `a ? b, c : d`
is legal. Common-type selection and null-pointer-constant support in
branches match the other binary-operator rules.

**Stage 19** added the comma operator at the lowest expression
precedence. The left operand is evaluated and its result discarded; the
right operand's value and type become the expression result. Function
argument lists and declarator lists continue to use the comma as a
separator, not an operator.

**Stage 20** refactored declarator parsing to separate
`parse_declarator` from `parse_type_specifier`, then used that
foundation to support comma-separated init-declarator lists
(`int a, *b = &a, c;`). Each declarator in the list may carry its own
optional initializer. Array declarators are rejected in multi-declarator
lists. Stage 20-03 added a comprehensive regression suite covering all
declaration forms. Aggregate: 630/630 tests pass.

## Out of Scope (Not Yet Implemented)

These features remain explicitly outside the implemented language and
will appear in future stages if and when their specs land:

- Structs, unions, enums.
- Floating-point types (`float`, `double`).
- Unsigned integer types.
- `typedef` and storage-class specifiers (`static`, `extern`, etc.).
- File-scope (global) variable declarations.
- `for`-loop initializer declarations (`for (int i = 0; …)`).
- Variadic functions.
- The C preprocessor.
- Function pointers.
- Octal (`'\123'`) and hex (`'\x41'`) character escapes.
- Multi-character constants (`'ab'`).
- Wide-character literals (`L'A'`, `u'A'`, `U'A'`).
- Initializer lists for non-`char` arrays; global array initialization.
- Calls with more than 6 arguments (calling-convention limit).
- Pointer subtraction (yielding `ptrdiff_t`).
- Multi-dimensional arrays.

## Architecture

```
src/
├── lexer.c              Tokenizer
├── parser.c             Recursive-descent parser, builds AST
├── ast.c                AST node lifecycle helpers
├── ast_pretty_printer.c --print-ast renderer
├── type.c               Type system (singletons + heap pointer chains)
├── codegen.c            Single-pass walker → NASM Intel-syntax asm
├── compiler.c           Driver
└── util.c               Misc helpers
```

The grammar is documented in `docs/grammar.md` and is updated
alongside any stage that touches it. A full parse-tree diagram (all
four grammar sections) is in `docs/other/stage-21-03-parse-tree.md`.

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
`test/print_ast`, `test/print_tokens`, and `test/print_asm`, each
driven by a `run_tests.sh` script.
