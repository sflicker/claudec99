# ClaudeC99 Project Status — Through Stage 12-04

_As of 2026-04-25 (commit `377763d`)_

## Overview

ClaudeC99 is a from-scratch C99 subset compiler written in C, targeting
x86_64 Linux via NASM (Intel syntax). The compiler is built incrementally
through small, spec-driven stages — each stage is a self-contained
specification followed by a kickoff, implementation, and milestone /
transcript record. Output is single-file assembly that is assembled with
`nasm -f elf64` and linked directly with `ld -e main` (no libc).

**Stages completed: 37** (stage-01 through stage-12-04).

## Build & Test

| Component               | Count          |
|-------------------------|----------------|
| Source files (`src/*.c`)| 8              |
| Header files (`include/`) | 8            |
| Total LOC (src + include) | 2,921        |
| Valid runtime tests     | 200            |
| Invalid (compile-error) tests | 22       |
| Print-AST golden tests  | 16             |
| **Total tests**         | **238**        |
| Git commits             | 111            |

All 238 tests pass with no regressions.

## Language Features Implemented

### Core program shape
- Translation unit with one or more external declarations.
- Function definitions and forward declarations.
- Multiple functions per translation unit; recursive calls.
- `main` is the entry point; non-`main` functions return via SysV
  AMD64 calling convention.

### Statements
- `return <expr>;`
- `if` / `else` (chained, with blocks).
- `while`, `do … while`, `for` (with optional init/cond/update).
- `switch` / `case` / `default` (linear dispatch; nested switches OK).
- `break`, `continue`, `goto` + labeled statement.
- Compound block statements with proper lexical scoping (shadowing
  permitted; duplicate declaration in the same scope rejected).
- Expression statements.
- Line `// …` and block `/* … */` comments.

### Declarations & types
- Integer base types: `char`, `short`, `int`, `long` (signed only).
- Pointer types of arbitrary depth: `int *`, `int **`, `char *`, etc.
- Local variables with initializers.
- Function parameters (typed) and typed return values.
- Forward declarations and definitions; signature consistency
  enforced at registration time.

### Expressions
- Integer literals with optional `L` / `l` suffix and overflow-aware
  literal typing (Stage 11-07).
- Variable reference; assignment (`=`, `+=`, `-=`); chained
  assignment.
- Arithmetic: `+`, `-`, `*`, `/` (signed integer).
- Equality / relational: `==`, `!=`, `<`, `<=`, `>`, `>=`.
- Logical: `&&`, `||`, `!` (with short-circuit evaluation).
- Prefix and postfix `++` / `--`.
- Casts: `(<integer-type>) <expr>`.
- Address-of `&` and dereference `*`; dereference as both rvalue and
  lvalue (`*p = …`); pointer-typed parameters and arguments.
- Function calls with up to 6 arguments (SysV register passing);
  argument-to-parameter conversion (widen / narrow / sign-extend) at
  the call boundary; return-value conversion at the function exit.

### Code generation
- Single-pass walk from AST to NASM Intel-syntax assembly.
- Per-function stack frames; locals laid out with natural alignment
  for their declared sizes (1 / 2 / 4 / 8 bytes).
- Integer promotions and common-type rules between operands of
  arithmetic / comparison operators.
- Pointer values stored / loaded as full 8-byte slots; pointer-store
  width selected by the pointed-to base type.

## Stage-by-Stage Timeline

| Stage     | Focus                                                |
|-----------|------------------------------------------------------|
| 01        | Minimal compiler — `return <int_literal>;`           |
| 02        | Integer arithmetic expressions                      |
| 03        | Equality / relational expressions                   |
| 04        | Unary operators, `if` / `else`, block statements    |
| 05        | Integer variables with initializers                 |
| 06        | `while` statement                                   |
| 07-01     | Assignment expressions                              |
| 07-02     | `for` statement                                     |
| 07-03     | Increment / decrement (`++`, `--`)                  |
| 07-04     | AST pretty-printer (`--print-ast`)                  |
| 08-01     | Logical operators (`&&`, `\|\|`) with short-circuit |
| 08-02     | Nested lexical scopes                               |
| 09-01     | Reorganize source into multiple files               |
| 09-02     | Grammar updates                                     |
| 09-03     | Multiple functions per translation unit             |
| 09-04     | Function calls                                      |
| 09-05     | Function forward declarations                       |
| 10-01     | `break` / `continue`                                |
| 10-02     | `do … while`                                        |
| 10-03-01  | `switch` / `default` skeleton                       |
| 10-03-02  | `case` and dispatch                                 |
| 10-03-03  | Standard `switch` semantics                         |
| 10-04     | `goto` and labeled statements                       |
| 10-05     | Line / block comments                               |
| 11-01     | Minimal type system (`char` / `short` / `int` / `long`) |
| 11-02     | Integer-type-aware codegen                          |
| 11-03     | Promotions and common arithmetic type               |
| 11-04     | Types for equality / logical / conditional          |
| 11-05-01  | Typed parameter & return grammar                    |
| 11-05-02  | Typed function signatures and call semantics        |
| 11-05-03  | Conversions at the function boundary                |
| 11-06     | Casting expressions                                 |
| 11-07     | Integer-literal typing with `L` suffix and overflow |
| 12-01     | Pointer types in declarations                       |
| 12-02     | Address-of `&` and dereference `*` (rvalue)         |
| 12-03     | Assignment through pointer (`*p = …`)               |
| **12-04** | **Pointer parameters and arguments (current)**      |

## Recently Shipped (Stage 12-04)

Pointer support reaches the function-call boundary. Functions may
declare pointer parameters (`int *p`, `int **pp`, etc.) and callers
may pass pointer-valued expressions (`&x`, pointer variables,
dereferences). Pointer parameters are stored as 8-byte locals so the
stage 12-02 / 12-03 dereference and assign-through-pointer machinery
work inside callees with no further change. The call site enforces
strict pointer-vs-integer mismatch and pointer-base-chain equality;
mismatched bases (`char *` to `int *`) and integer/pointer
mismatches are rejected with diagnostics.

Concretely:
- `<parameter_declaration>` is now `<type> <identifier>` instead of
  `<integer_type> <identifier>`.
- `pointer_types_equal` helper walks both chains for compatibility
  checks at call sites.
- 3 valid + 3 invalid + 1 print-AST tests added.

## Out of Scope (Not Yet Implemented)

These features remain explicitly outside the implemented language and
will appear in future stages if and when their specs land:

- Pointer arithmetic, pointer comparisons.
- Null pointer constants.
- Arrays and array indexing.
- Strings / string literals.
- Structs, unions, enums.
- Floating-point types.
- Unsigned integer types.
- `typedef` and storage-class specifiers (`static`, `extern`, etc.).
- Variadic functions.
- The C preprocessor.
- Pointer type conversions / casts between incompatible pointer types.
- Pointer return types.
- Function pointers.
- More than 6 function arguments (calling-convention limit).

## Architecture

```
src/
├── lexer.c          Tokenizer
├── parser.c         Recursive-descent parser, builds AST
├── ast.c            AST node lifecycle helpers
├── ast_pretty_printer.c  --print-ast renderer
├── type.c           Type system (singletons + heap pointer chains)
├── codegen.c        Single-pass walker → NASM Intel-syntax asm
├── compiler.c       Driver
└── util.c           Misc helpers
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

Tests live next to the runners in `test/valid`, `test/invalid`, and
`test/print_ast`, each driven by a `run_tests.sh` script.
