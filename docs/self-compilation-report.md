# Self-Compilation Diagnostic Report

**Date:** 2026-05-29
**Compiler:** `build/ccompiler`
**Flags:** `--max-errors=0 -Iinclude -Itest/include`

Each `src/*.c` file was compiled independently, in alphabetical order, with the
project's own built compiler. Every reported error was traced to its **root
cause**; the long runs of `expected type specifier` that follow each root cause
are parser-recovery cascades and are not counted as distinct gaps.

There were **no** `include file not found` errors, so **Category A is empty** —
every stub header the sources need already exists in `test/include/`. All
failures are Category B (unimplemented language features) plus one fixed-size
internal-table limit.

## Summary

| Module | Result | Root cause category |
|---|---|---|
| `ast.c` | FAIL | B3 subscript-of-member; B1 `for`-init |
| `ast_pretty_printer.c` | FAIL | B1 `for`-init; B2 enum `case` labels |
| `codegen.c` | FAIL | B9/B8 header (`codegen.h`); B2 enum `case`; B1 `for`-init; L1 too-many-functions |
| `compiler.c` | FAIL | B7/B8/B9/B11 headers (`codegen.h`,`lexer.h`,`parser.h`,`util.h`); B2; B1 |
| `lexer.c` | FAIL | B7 header (`lexer.h` `const` member); B6 hex/octal char escape |
| `parser.c` | FAIL | B7/B11 headers (`lexer.h`,`parser.h`,`util.h`); B1; B2 |
| `preprocessor.c` | FAIL | B1 `for`-init; B3 subscript-of-member; B4 compound-assign; B5 postfix `++`; B6 hex/octal string escape; L1 too-many-functions |
| `type.c` | FAIL | B1 `for`-init; B2 enum `case` labels |
| `util.c` | FAIL | B11 `__attribute__` (`util.h` + `util.c`) |
| `version.c` | **PASS** | — |

---

## Category A — Missing stub system headers

None. No `include file not found` diagnostics were emitted by any module; the
existing stubs under `test/include/` cover every system header the sources
include.

---

## Category B — Language features not yet implemented

Each subsection gives the reproduced trigger, the diagnostic, the C99 clause,
and the affected `file:line` sites (root cause only; cascades omitted).

### B1 — `for`-init declarations  ·  C99 §6.8.5.3

`for (int i = 0; …)` is rejected. The declaration in the first clause of a
`for` is not parsed; the parser expects an expression instead.

- Keyword type → `error: expected expression, got 'int'`
- `typedef`-name type (e.g. `size_t`) → `error: expected ';', got identifier ('i')`
  (the type name is consumed as the init *expression*, then `i` is unexpected)

Verified: `for (int i=0;…)` fails; `for (size_t i=0;…)` fails; an equivalent
loop with the declaration hoisted out compiles.

| File:line | Form |
|---|---|
| `src/ast.c:27` | `for (int i = …)` |
| `src/ast_pretty_printer.c:29` | `for (int i = …)` |
| `src/codegen.c:124, 156, 654, 870` | `for (int i = …)` |
| `src/compiler.c:130, 267` | `for (int i = …)` |
| `src/parser.c:133, 1079` | `for (int i = …)` |
| `src/preprocessor.c:124, 233, 268` | `for (int i = …)` |
| `src/preprocessor.c:91, 104, 162, 412` | `for (size_t i = …)` |
| `src/type.c:95` | `for (int i = …)` |

### B2 — Enum / non-literal constant `case` labels  ·  C99 §6.8.4.2

`case <enum-constant>:` is rejected; the label parser accepts only an integer
literal.

Diagnostic: `error: expected integer literal, got identifier ('…')`
Verified with `enum E{A,B}; switch(e){case A: … case B: …}`.

| File:line | Label |
|---|---|
| `src/ast_pretty_printer.c:69` | `case AST_TRANSLATION_UNIT:` |
| `src/codegen.c:10, 134, 408, 436` | `case TYPE_VOID:` / `case TYPE_CHAR:` |
| `src/compiler.c:23` | `case TOKEN_EOF:` |
| `src/type.c:147, 175` | `case TYPE_VOID:` / `case TYPE_BOOL:` |

### B3 — Subscript of a member-access base  ·  C99 §6.5.2.1

`p->arr[i]` and `s.arr[i]` are rejected; the subscript base must be a bare
identifier.

Diagnostic: `error: subscript base must be an identifier`
Verified: `p->a[i]` fails; `a[i]` compiles.

| File:line | Expression |
|---|---|
| `src/ast.c:21` | `parent->children[parent->child_count++]` |
| `src/preprocessor.c:353, 485` | `arg.data[arg.len - 1]` |

### B4 — Compound assignment to a non-identifier lvalue  ·  C99 §6.5.16.2

`g->cap *= 2;` and `g->len += n;` are rejected; compound-assignment operators
are accepted only when the left operand is a plain identifier.

Diagnostic: `error: expected ';', got '*='` / `'+='`
Verified: `x *= 2` (identifier) compiles; `g->cap *= 2` (member) fails.

| File:line | Expression |
|---|---|
| `src/preprocessor.c:36` | `g->cap *= 2;` |
| `src/preprocessor.c:50` | `g->len += n;` |

### B5 — Postfix `++`/`--` on a non-identifier operand  ·  C99 §6.5.2.4

`s[0]++` and `m->n++` are rejected; postfix increment/decrement requires a bare
identifier operand.

Diagnostic: `error: postfix ++ requires an identifier`
Verified: `i++` (identifier) and `(*p)++` (dereference) compile; `s[0]++`
(subscript) and `m->n++` (member) fail.

| File:line | Expression |
|---|---|
| `src/preprocessor.c:735, 844, 907, 932, 967, 992, 1012, 1032, 1052` | postfix on subscript/member operand |
| `src/preprocessor.c:823, 882` | `s[(*in)++]` form |

### B6 — Hexadecimal and octal escape sequences  ·  C99 §6.4.4.4

`'\xNN'` and `'\NNN'` (and their string-literal forms) are rejected. Simple
escapes (`\n \t \0 \a \b \f \v \r \?`) are accepted.

Diagnostics (emitted by the lexer, without a position):
- `error: invalid escape sequence in character literal`
- `error: invalid escape sequence in string literal`

Verified: `'\x41'` and `'\101'` both fail; the named single-char escapes pass.

| File | Use |
|---|---|
| `src/lexer.c` | char literal `'\x01'` (e.g. line 100) — char-literal diagnostic |
| `src/preprocessor.c` | string literal `"\x01"` (lines 1382, 1393) — string-literal diagnostic |

### B7 — `const`-qualified struct members  ·  C99 §6.7.2.1 / §6.7.3

A `const`-qualified member inside a `struct`/`union` declaration is rejected.
(`const` on a *function parameter* is accepted, so this is specific to member
declarations.)

Diagnostic: `error: expected integer type, got 'const'`
Verified: `struct S { const char *p; };` fails.

| Header:line | Member | Blocks |
|---|---|---|
| `include/lexer.h:7` | `const char *source;` (struct `Lexer`) | `lexer.c`, `parser.c`, `compiler.c` |
| `include/codegen.h` | `const char *current_func;` (struct `CodeGen`) | `codegen.c`, `compiler.c` |

Because the enclosing typedef struct fails to parse, `Lexer`/`CodeGen` become
unknown type names everywhere they are used, and the would-be struct members
leak into global scope — this is the origin of the cascade
`duplicate global declaration 'switch_depth'` seen in `parser.h:83`.

### B8 — Multidimensional (array-of-array) members  ·  C99 §6.7.2.1

A member declared as `T name[N][M];` is rejected after the first dimension.

Diagnostic: `error: expected ';', got '['`
Verified: `struct S { char m[4][8]; };` fails.

| Header:line | Member | Blocks |
|---|---|---|
| `include/codegen.h:121` | `char user_labels[MAX_USER_LABELS][256];` (struct `CodeGen`) | `codegen.c`, `compiler.c` |

### B9 — Redundant typedef of an already-typedef'd name  ·  C99 §6.7

`typedef struct ASTNode ASTNode;` is rejected when `ASTNode` is already a
typedef (the common forward-declaration idiom). C99 permits redeclaring a
typedef name to a compatible type within the same scope.

Diagnostic: `error: duplicate typedef '…' in this scope`
Verified: two identical `typedef struct N N;` lines fail on the second.

| Header:line | Declaration | Blocks |
|---|---|---|
| `include/codegen.h:94` | `typedef struct ASTNode ASTNode;` (already in `ast.h`) | `codegen.c`, `compiler.c` |

### B11 — `__attribute__((…))` GNU attribute specifier

A declaration prefixed with `__attribute__((…))` is rejected; the attribute is
not recognized, so the declaration is read as having no type specifier.

Diagnostic: `error: expected type specifier`
Verified: `__attribute__((noreturn)) void f(void);` fails.

| File:line | Site | Blocks |
|---|---|---|
| `include/util.h:22, 26, 31` | `__attribute__((noreturn, format(printf, …)))` on error/warning prototypes | `util.c`, `codegen.c`, `compiler.c`, `parser.c` |
| `src/util.c:14, 28, 37` | `__attribute__((noreturn))` on the definitions |

---

## Implementation limit (not a syntax gap)

### L1 — Fixed function-table capacity: `max 64`

Large modules exceed a hard-coded limit of 64 functions per translation unit.
This is a capacity limit in a fixed-size internal table, not a rejected
construct.

Diagnostic: `error: too many functions (max 64)`

| File:line |
|---|
| `src/codegen.c:928, 3710, 3724, 3770, 3864` |
| `src/preprocessor.c:1068` |

---

## Successful compilation

| Module | Why it succeeded |
|---|---|
| `src/version.c` | Tiny module: no `for`-init loops, no `switch` on enums, no member-base subscripts, no compound assignment to members, no hex/octal escapes, and it does not include the headers that carry the `const`-member, 2D-array, duplicate-typedef, or `__attribute__` constructs. It uses only features the compiler already supports. |

---

## Feature gap summary

| Gap | C99 section | Affected modules |
|---|---|---|
| B1 `for`-init declarations | §6.8.5.3 | ast, ast_pretty_printer, codegen, compiler, parser, preprocessor, type |
| B2 enum/constant `case` labels | §6.8.4.2 | ast_pretty_printer, codegen, compiler, type |
| B3 subscript of member-access base | §6.5.2.1 | ast, preprocessor |
| B4 compound assignment to member lvalue | §6.5.16.2 | preprocessor |
| B5 postfix `++`/`--` on non-identifier | §6.5.2.4 | preprocessor |
| B6 hex/octal escape sequences | §6.4.4.4 | lexer, preprocessor |
| B7 `const`-qualified struct members | §6.7.2.1 / §6.7.3 | lexer.h→(lexer, parser, compiler); codegen.h→(codegen, compiler) |
| B8 multidimensional array members | §6.7.2.1 | codegen.h→(codegen, compiler) |
| B9 redundant typedef of same name | §6.7 | codegen.h→(codegen, compiler) |
| B11 `__attribute__` specifier | (GNU ext.) | util.h→(util, codegen, compiler, parser) |
| L1 function-table capacity (max 64) | impl. limit | codegen, preprocessor |
