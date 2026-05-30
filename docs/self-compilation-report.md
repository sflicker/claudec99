# Self-Compilation Diagnostic Report

**Date:** 2026-05-30
**Compiler:** `build/ccompiler`
**Flags:** `--max-errors=0 -Iinclude -Itest/include`

## Summary

Each `src/*.c` file was compiled independently with the project's own
compiler. Every required system header already has a stub in
`test/include/`, so **there are no Category A failures** — every failure is a
Category B unimplemented C99 (or common GNU extension) feature. One module
(`version.c`) compiles cleanly.

| Module | Result | Root cause category |
|---|---|---|
| `ast.c` | FAIL | B — member-access subscript base |
| `ast_pretty_printer.c` | FAIL | B — member-access subscript base |
| `codegen.c` | FAIL | B — redundant typedef (`ASTNode`); also `__attribute__`, subscript base |
| `compiler.c` | FAIL | B — redundant typedef (`ASTNode`); also `const` member, `__attribute__`, subscript base |
| `lexer.c` | FAIL | B — `const`-qualified struct member |
| `parser.c` | FAIL | B — `const`-qualified struct member; also `__attribute__`, subscript base |
| `preprocessor.c` | FAIL | B — compound assignment / postfix `++` / subscript on non-identifier |
| `type.c` | FAIL | B — member-access subscript base |
| `util.c` | FAIL | B — `__attribute__` GNU extension |
| `version.c` | **PASS** | — |

---

## Category A — Missing stub system headers

**None.** No `include file not found` diagnostics were produced by any module.
Every `#include <...>` resolved against an existing stub in `test/include/`
(`ctype.h`, `errno.h`, `limits.h`, `setjmp.h`, `stdarg.h`, `stdbool.h`,
`stddef.h`, `stdint.h`, `stdio.h`, `stdlib.h`, `string.h`, `time.h`).

---

## Category B — Language features not yet implemented

Each subsection below isolates one **root cause**. A single root-cause
rejection typically derails the parser for the rest of the declaration or
function, producing a long cascade of follow-on diagnostics — most commonly
`expected type specifier`, `unknown type name 'X'`, `non-constant initializer
for global 'X'`, `too many functions (max 64)`, and stray `invalid escape
sequence` reports. Those cascades (the bulk of the 556 total errors) are
**not** independent bugs; only the root causes are catalogued here.

### B1 — Subscript of a member-access (`->` / `.`) expression

The subscript operator only accepts a bare identifier as its base; indexing
the result of a member access is rejected.

```c
parent->children[parent->child_count++] = child;   // ast.c:21
//      ^ "subscript base must be an identifier"
```

Minimal reproduction:

```c
struct S { int *a; };
void f(struct S *p) { p->a[0] = 1; }   // error: subscript base must be an identifier
```

C99 §6.5.2.1 — the postfix-expression preceding `[` may be any
postfix-expression, not just an identifier. This is the single most pervasive
gap, appearing in seven modules.

| Location | Expression |
|---|---|
| `src/ast.c:21`, `:28` | `parent->children[...]` |
| `src/ast_pretty_printer.c:127` | `node->value[0]` |
| `src/codegen.c:125`, `:157`, `:655`, `:871` | member-array subscript |
| `src/compiler.c:131` | member-array subscript |
| `src/parser.c:134` | member-array subscript |
| `src/preprocessor.c:92`, `:105`, `:153`, `:163`, `:353`, `:485` | member-array subscript |
| `src/type.c:96` | `t->params[i]` |

### B2 — `const` qualifier on a struct / union member

A `const`-qualified member declaration is rejected. (Notably, `const` on a
local, parameter, global, or typedef **does** compile — the gap is specific to
aggregate members.)

```c
typedef struct {
    const char *source;   // include/lexer.h:7
//  ^ "expected integer type, got 'const'"
    ...
} Lexer;
```

Minimal reproduction:

```c
struct S { const int x; };   // error: expected integer type, got 'const'
```

C99 §6.7.2.1 (struct/union members) with the type-qualifier of §6.7.3. Because
the failure is in `include/lexer.h`, it blocks every module that includes it:
`lexer.c`, `parser.c`, and `compiler.c`.

| Location | Member |
|---|---|
| `include/lexer.h:7` | `const char *source;` (via `lexer.c`, `parser.c`, `compiler.c`) |

### B3 — Compound assignment with a non-identifier left operand

Compound assignment operators (`*=`, `+=`, …) are parsed only when the left
operand is a bare identifier; a member-access lvalue terminates the statement
early, so the operator is seen where a `;` is expected.

```c
g->cap *= 2;   // preprocessor.c:36  -> error: expected ';', got '*='
g->len += 1;   // preprocessor.c:50  -> error: expected ';', got '+='
```

Minimal reproduction:

```c
struct S { int c; };
void f(struct S *g) { g->c *= 2; }   // error: expected ';', got '*='
// but:  int c; c *= 2;              // compiles
```

C99 §6.5.16.2 — the left operand of a compound assignment is any modifiable
lvalue.

| Location | Expression |
|---|---|
| `src/preprocessor.c:36` | `g->cap *= 2;` |
| `src/preprocessor.c:50` | `g->len += …;` |

### B4 — Postfix `++` / `--` on a non-identifier operand

Like B3, the postfix increment/decrement operators require a bare identifier
operand and reject a member-access lvalue.

```c
... node->value[len++] ...   // preprocessor.c (11 sites)
//  error: postfix ++ requires an identifier
```

Minimal reproduction:

```c
struct S { int c; };
void f(struct S *g) { g->c++; }   // error: postfix ++ requires an identifier
```

C99 §6.5.2.4 — the operand of postfix `++`/`--` is any modifiable lvalue.

| Location |
|---|
| `src/preprocessor.c:735, 823, 844, 882, 907, 932, 967, 992, 1012, 1032, 1052` |

### B5 — Redundant typedef redeclaration

A forward typedef that repeats a name already typedef'd in another header is
rejected as a duplicate. `ASTNode` is defined by `include/ast.h:64` (`typedef
struct ASTNode { … } ASTNode;`) and re-declared compatibly by
`include/codegen.h:93` (`typedef struct ASTNode ASTNode;`).

```c
typedef struct N N;
typedef struct N N;   // error: duplicate typedef 'N' in this scope
```

C11 §6.7p3 explicitly permits a typedef to be redeclared to the same type;
C99 omits this allowance, but the redundant-typedef idiom is universally
accepted by production compilers and is the established way to share an
incomplete-type alias across headers. Blocks `codegen.c` and `compiler.c`.

| Location | Name |
|---|---|
| `include/codegen.h:94` (vs `include/ast.h:64`) | `ASTNode` |

### B6 — `__attribute__((…))` GNU extension

The compiler does not recognise the `__attribute__` specifier and reads it as a
stray declaration, emitting `expected type specifier`.

```c
__attribute__((noreturn, format(printf, 1, 2)))   // include/util.h:22
void compile_error(const char *fmt, ...);
```

Minimal reproduction:

```c
__attribute__((noreturn)) void f(void);   // error: expected type specifier
```

Not part of C99 (a GNU extension), but used throughout `include/util.h`
(lines 22, 26, 31, …). Because nearly every module pulls in `util.h`, this
blocks `util.c`, `parser.c`, `codegen.c`, and `compiler.c`.

| Location |
|---|
| `include/util.h:22, 26, 31` (via `util.c`, `parser.c`, `codegen.c`, `compiler.c`) |

---

## Successful compilation

| Module | Why it compiled |
|---|---|
| `version.c` | It includes only `version.h` and `<stdio.h>`, neither of which uses any of the six gaps above. Its body performs only plain `printf` calls and simple returns — no member-access subscripting, compound assignment on members, `const` members, redundant typedefs, or `__attribute__`. |

Every other module transitively includes at least one of `lexer.h` (B2),
`codegen.h` (B5), or `util.h` (B6), and/or directly uses member-access
subscripting (B1), so all nine fail.

---

## Feature gap summary

| Gap | C99 / spec section | Affected modules |
|---|---|---|
| B1 — subscript base may be any postfix-expr (`p->a[i]`) | §6.5.2.1 | `ast.c`, `ast_pretty_printer.c`, `codegen.c`, `compiler.c`, `parser.c`, `preprocessor.c`, `type.c` |
| B2 — `const`-qualified struct/union member | §6.7.2.1, §6.7.3 | `lexer.c`, `parser.c`, `compiler.c` (via `lexer.h`) |
| B3 — compound assignment on non-identifier lvalue | §6.5.16.2 | `preprocessor.c` |
| B4 — postfix `++`/`--` on non-identifier lvalue | §6.5.2.4 | `preprocessor.c` |
| B5 — redundant typedef redeclaration | §6.7 (C11 §6.7p3) | `codegen.c`, `compiler.c` (via `codegen.h`) |
| B6 — `__attribute__((…))` specifier | GNU ext. (not in C99) | `util.c`, `parser.c`, `codegen.c`, `compiler.c` (via `util.h`) |

**Highest-leverage fixes:** B1 (member-access subscript) unblocks the most
modules and is pure parser work shared with B3/B4 — all three stem from the
same "operand/base must be an identifier" restriction on postfix and
assignment expressions. Generalising that single restriction to accept any
lvalue/postfix-expression would clear B1, B3, and B4 at once.
