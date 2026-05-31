# Self-Compilation Diagnostic Report

**Date:** 2026-05-30
**Compiler:** `build/ccompiler`
**Flags:** `--max-errors=0 -Iinclude -Itest/include`

## Summary

| Module | Result | Root cause category |
|---|---|---|
| `ast.c` | FAIL | B — postfix `++` on a member-access lvalue |
| `ast_pretty_printer.c` | FAIL | A — `putchar` missing from `stdio.h` stub |
| `codegen.c` | FAIL | B — duplicate `typedef ASTNode` (+ function-table capacity limit) |
| `compiler.c` | FAIL | B — cascade of B gaps from `codegen.h`/`lexer.h`/`parser.h`/`util.h` |
| `lexer.c` | FAIL | B — `const` struct member + `\xNN` hex escape |
| `parser.c` | FAIL | B — cascade (`Lexer`/`Parser` undefined via `lexer.h`/`util.h`) |
| `preprocessor.c` | FAIL | B — postfix/prefix `++`/`--` on lvalues, `\xNN` escape, string concatenation |
| `type.c` | **PASS** | — |
| `util.c` | FAIL | B — GNU `__attribute__` specifier |
| `version.c` | **PASS** | — |

**8 of 10 modules failed; 2 passed.**

All failures trace to **6 distinct unimplemented language features (Category B)**
plus **1 missing stub declaration (Category A)**. No failure is caused by an
entirely missing system header — every needed header has a stub in
`test/include/`.

---

## Category A — Missing stub system headers

### `putchar` — incomplete `stdio.h` stub

```
error: call to undefined function 'putchar'
```

`test/include/stdio.h` exists, but it does **not** declare `putchar`.
`src/ast_pretty_printer.c` calls `putchar` at lines 30, 137 and 158, so the
compiler treats it as an implicit/undefined function and rejects the call.

This is the only Category A issue, and it is a *missing declaration in an
existing stub* rather than a wholly missing header. Adding

```c
int putchar(int c);
```

to `test/include/stdio.h` resolves `ast_pretty_printer.c` entirely (it has no
other root-cause errors).

| File:line | Call |
|---|---|
| `src/ast_pretty_printer.c:30` | `putchar(' ')` |
| `src/ast_pretty_printer.c:137` | `putchar(b)` |
| `src/ast_pretty_printer.c:158` | `putchar(b)` |

---

## Category B — Language features not yet implemented

### B1 — Postfix/prefix `++` / `--` on non-identifier lvalues — C99 §6.5.2.4, §6.5.3.1

```
error: postfix ++ requires an identifier
error: prefix -- requires an identifier
```

The increment/decrement operators only accept a bare identifier operand. Any
member-access or subscript lvalue (`p->len++`, `arr[i]++`, `--x->n`) is
rejected. Each rejection cascades into a string of `expected type specifier`
errors on the following lines of the same function.

The root cause in `ast.c`:

```c
parent->children[parent->child_count++] = child;   /* ast.c:21 */
```

and in `preprocessor.c`:

```c
g->data[g->len++] = c;                              /* preprocessor.c:40 */
```

| File:line | Construct |
|---|---|
| `src/ast.c:21` | `parent->child_count++` |
| `src/preprocessor.c:40` | `g->len++` |
| `src/preprocessor.c:158` | postfix `++` on member |
| `src/preprocessor.c:172` | prefix `--` on member |
| `src/preprocessor.c:355` | postfix `--` on member |
| `src/preprocessor.c:486, 735, 823, 844, 882, 907, 932, 967, 992, 1012, 1032, 1052` | postfix `++`/`--` on member/subscript |

(Counts are root causes only; each spawns a cascade of `expected type
specifier` errors that are omitted.)

### B2 — `const` type qualifier on struct member declarations — C99 §6.7.3

```
include/lexer.h:7:5: error: expected integer type, got 'const'
```

A `const`-qualified member inside a struct is not accepted:

```c
typedef struct {
    const char *source;   /* lexer.h:7 — rejected */
    ...
} Lexer;
```

Because the `Lexer` struct fails to parse, every later reference to `Lexer`
reports `unknown type name 'Lexer'`, which in turn breaks `Parser` (it holds a
`Lexer *`). This single gap blocks `lexer.c`, `parser.c` and `compiler.c`.

| File:line | Member |
|---|---|
| `include/lexer.h:7` | `const char *source;` |

Affected (transitively): `src/lexer.c`, `src/parser.c`, `src/compiler.c`.

### B3 — GNU `__attribute__` specifier — (GNU extension; not C99)

```
include/util.h:22:1: error: expected type specifier
```

`util.h` decorates its declarations with `__attribute__`, which the compiler
does not recognize as a declaration prefix:

```c
__attribute__((noreturn, format(printf, 1, 2)))
void compile_error(const char *fmt, ...);          /* util.h:22 */
```

| File:line | Attribute |
|---|---|
| `include/util.h:22` | `__attribute__((noreturn, format(printf,1,2)))` |
| `include/util.h:26` | `__attribute__((noreturn, format(printf,4,5)))` |
| `include/util.h:31` | `__attribute__((format(printf,4,5)))` |

Affected (transitively): `src/util.c`, `src/parser.c`, `src/codegen.c`,
`src/compiler.c`.

### B4 — Redundant `typedef` of the same name in the same scope — C99 §6.7 (permitted in C11 §6.7p3)

```
include/codegen.h:94:1: error: duplicate typedef 'ASTNode' in this scope
```

`ast.h` (pulled in by `codegen.h`) already defines

```c
typedef struct ASTNode { ... } ASTNode;            /* ast.h:64 */
```

and `codegen.h` repeats the forward form:

```c
typedef struct ASTNode ASTNode;                    /* codegen.h:94 */
```

The compiler rejects the second, identical typedef. The failure aborts parsing
of the rest of `codegen.h`, so the following `SwitchCtx`/`CodeGen` structs are
never registered → the long cascade of `unknown type name 'CodeGen'` /
`unknown type name 'SwitchCtx'` in `codegen.c` and `compiler.c`.

| File:line | Construct |
|---|---|
| `include/codegen.h:94` | `typedef struct ASTNode ASTNode;` |

Affected (transitively): `src/codegen.c`, `src/compiler.c`.

### B5 — `\xNN` hex escape sequences — C99 §6.4.4.4

```
error: invalid escape sequence in character literal
error: invalid escape sequence in string literal
```

The lexer rejects hexadecimal escape sequences in both character and string
literals. The codebase uses `\x01` as an internal source-location marker:

```c
if (c == '\x01') { ... }                            /* lexer.c:100 (char) */
snprintf(buf, n, "\x01" "1:%s\n", include_path);    /* preprocessor.c:1382 (string) */
```

| File:line | Literal |
|---|---|
| `src/lexer.c:67, 69, 100` | `'\x01'` (character) |
| `src/preprocessor.c:1382, 1393` | `"\x01"` (string) |

### B6 — Adjacent string-literal concatenation — C99 §6.4.5

```
src/preprocessor.c:604:33: error: expected ')', got string literal
```

Two string-literal tokens written next to each other are not concatenated into
one; after the first literal the parser expects `)` or `,` and instead finds a
second string:

```c
fprintf(stderr,
        "error: argument count mismatch for macro '%.*s':"
        " expected %s%d, got %d\n",                 /* preprocessor.c:603-604 */
        ...);
```

| File:line | Construct |
|---|---|
| `src/preprocessor.c:603-604` | adjacent `"..."` `"..."` |

(The `"\x01" "1:%s\n"` forms at `preprocessor.c:1382/1393` are also adjacent
concatenations, but the `\x` escape — B5 — is reported first there.)

### Note — compiler capacity limit (not a language gap)

```
src/codegen.c:983:70: error: too many functions (max 64)
src/preprocessor.c:1068:70: error: too many functions (max 64)
```

`codegen.c` and `preprocessor.c` exceed a fixed internal table size of 64
functions per translation unit. This is a compiler **capacity** limit, not an
unimplemented C99 feature, but it would block these two modules even after the
B-category gaps are closed. Raising the function-table bound is required for
full self-compilation.

---

## Successful compilation

| Module | Why it compiled |
|---|---|
| `src/type.c` | Includes only `type.h`/`ast.h` (no `lexer.h`, `util.h`, `codegen.h`), declares no `const` members, uses no `\x` escapes or string concatenation, and increments only plain identifiers. → `type.asm` |
| `src/version.c` | Tiny module with no problematic constructs and no headers carrying B-category gaps. → `version.asm` |

Both produced assembly output and exited 0.

---

## Feature gap summary

| Gap | C99 section | Affected modules |
|---|---|---|
| Postfix/prefix `++`/`--` on member/subscript lvalues | §6.5.2.4, §6.5.3.1 | `ast.c`, `preprocessor.c` |
| `const` qualifier on struct member declarations | §6.7.3 | `lexer.c`, `parser.c`, `compiler.c` |
| GNU `__attribute__` specifier | (GNU ext.) | `util.c`, `parser.c`, `codegen.c`, `compiler.c` |
| Redundant `typedef` of same name in same scope | §6.7 (C11 §6.7p3) | `codegen.c`, `compiler.c` |
| `\xNN` hex escape sequences | §6.4.4.4 | `lexer.c`, `preprocessor.c` |
| Adjacent string-literal concatenation | §6.4.5 | `preprocessor.c` |
| *(capacity)* function table `max 64` | — | `codegen.c`, `preprocessor.c` |
| *(stub)* `putchar` declaration | — (Category A) | `ast_pretty_printer.c` |
