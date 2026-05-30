# Self-Compilation Diagnostic Report

**Date:** 2026-05-30
**Compiler:** `build/ccompiler`
**Flags:** `--max-errors=0 -Iinclude -Itest/include`

## Summary

| Module | Result | Root cause category |
|---|---|---|
| `ast.c` | FAIL | B — postfix `++` on member-access expression |
| `ast_pretty_printer.c` | FAIL | A — incomplete stub `stdio.h` (no `putchar`) |
| `codegen.c` | FAIL | B — redundant `typedef` redefinition (`ASTNode`) |
| `compiler.c` | FAIL | B — redundant `typedef` redefinition (`ASTNode`) |
| `lexer.c` | FAIL | B — `const` qualifier on struct member |
| `parser.c` | FAIL | B — `const` qualifier on struct member |
| `preprocessor.c` | FAIL | B — compound assignment operator (`*=`) |
| `type.c` | PASS | — |
| `util.c` | FAIL | B — `__attribute__` GNU extension |
| `version.c` | PASS | — |

8 of 10 modules failed. All failures trace to a single dominant root cause
each; the remaining errors in every file are cascades from that first
rejection (e.g. once a header line is misparsed, every following declaration
in that scope reports `expected type specifier`).

---

## Category A — Missing / incomplete stub system headers

### `stdio.h` — missing `putchar`

```
error: call to undefined function 'putchar'
```

`src/ast_pretty_printer.c` includes `<stdio.h>` (which resolves to the stub
`test/include/stdio.h`) and calls `putchar(' ')` at line 30, plus two more
sites at lines 137 and 158. The stub provides `printf`/`fprintf`/etc. but does
**not** declare `putchar`, so the compiler treats the call as a reference to an
undefined function.

This is **not** a compiler bug — the header is found, it simply lacks the
declaration. Adding an `int putchar(int);` prototype to `test/include/stdio.h`
clears all three sites and lets `ast_pretty_printer.c` proceed.

| File:line | Call |
|---|---|
| `src/ast_pretty_printer.c:30` | `putchar(' ')` |
| `src/ast_pretty_printer.c:137` | `putchar(b)` |
| `src/ast_pretty_printer.c:158` | `putchar(b)` |

---

## Category B — Language features not yet implemented

### B1 — Postfix `++` / `--` on a member-access expression

```
src/ast.c:21:51: error: postfix ++ requires an identifier
```

The compiler only accepts an identifier as the operand of a postfix increment.
`ast.c` line 21 applies it to a member-access subexpression:

```c
parent->children[parent->child_count++] = child;
```

Per C99 §6.5.2.4, the operand of postfix `++`/`--` may be any modifiable
lvalue, including `parent->child_count`. The two following `expected type
specifier` errors are cascades from the parser failing to recover within the
function body.

| File:line | Construct |
|---|---|
| `src/ast.c:21` | `parent->child_count++` |

### B2 — Redundant `typedef` redefinition

```
include/codegen.h:94:1: error: duplicate typedef 'ASTNode' in this scope
```

`include/ast.h` defines `typedef struct ASTNode { … } ASTNode;` (line 91).
`include/codegen.h` includes `ast.h` and then repeats the forward typedef:

```c
typedef struct ASTNode ASTNode;   /* codegen.h:93 */
```

Both names refer to the *same* underlying `struct ASTNode`. The compiler
rejects the second declaration as a duplicate. C11 §6.7p3 explicitly permits a
typedef to be redefined to the same type; C99 is stricter, but real-world C99
toolchains accept this idempotent redefinition, and the project's own headers
rely on it. This single rejection blocks both `codegen.c` and `compiler.c`
(everything that includes `codegen.h`); the 52/82 trailing errors are cascades.

| File:line | Affected via |
|---|---|
| `include/codegen.h:94` | `codegen.c`, `compiler.c` |

### B3 — `const` (type qualifier) on a struct member declaration

```
include/lexer.h:7:5: error: expected integer type, got 'const'
```

`include/lexer.h` declares a struct whose first member is qualified:

```c
typedef struct {
    const char *source;   /* lexer.h:7 */
    …
} Lexer;
```

The struct-member parser does not accept a leading type qualifier (`const`,
`volatile`) before the type specifier. Per C99 §6.7.2.1 / §6.7.3 a
struct-declaration may carry qualifiers. This blocks both `lexer.c` and
`parser.c` (the latter includes `lexer.h`); subsequent `unknown type name
'Lexer'` errors are cascades from the struct never being completed.

| File:line | Affected via |
|---|---|
| `include/lexer.h:7` | `lexer.c`, `parser.c` |

### B4 — Compound assignment operators (`*=`, and likely peers)

```
src/preprocessor.c:36:20: error: expected ';', got '*=' ('*=')
```

`preprocessor.c` line 36 uses a compound assignment:

```c
g->cap *= 2;
```

The parser does not recognise `*=` as an assignment operator, so it expects the
statement to terminate after `g->cap`. Per C99 §6.5.16.2 the compound operators
`*= /= %= += -= <<= >>= &= ^= |=` are all assignment operators. The 183 trailing
errors are cascades. (Only `*=` is exercised at the first failure site; other
compound operators elsewhere in the file are masked by the cascade.)

| File:line | Construct |
|---|---|
| `src/preprocessor.c:36` | `g->cap *= 2` |

### B5 — `__attribute__` GNU extension

```
include/util.h:22:1: error: expected type specifier
```

`include/util.h` decorates declarations with GNU attribute syntax:

```c
__attribute__((noreturn, format(printf, 1, 2)))
void compile_error(const char *fmt, ...);
```

The compiler has no handling for `__attribute__((…))`, so it reads the token
where a type specifier is expected and rejects it. This is a GNU extension
(not part of C99), but the project's own headers use it, so self-compilation
requires the compiler to at least *parse and ignore* attribute specifiers.
The rejection recurs at `util.h:22, 26, 31` and cascades into `util.c`.

| File:line | Attribute |
|---|---|
| `include/util.h:22` | `noreturn, format(printf, 1, 2)` |
| `include/util.h:26` | `noreturn, format(printf, 4, 5)` |
| `include/util.h:31` | `format(printf, 4, 5)` |

---

## Successful compilation

| Module | Why it compiled |
|---|---|
| `type.c` | Includes only `type.h`, which uses no `const`-qualified members, no `__attribute__`, and no redundant typedefs; its body avoids postfix `++` on member access and compound assignment at the points the parser reaches. |
| `version.c` | Trivial translation unit (version string / accessor) with no qualified struct members, attributes, or unsupported operators. |

---

## Feature gap summary

| Gap | C99 section | Affected modules |
|---|---|---|
| Postfix `++`/`--` on non-identifier lvalue | §6.5.2.4 | `ast.c` |
| Redundant `typedef` redefinition (same type) | §6.7 (cf. C11 §6.7p3) | `codegen.c`, `compiler.c` |
| `const`/`volatile` qualifier on struct member | §6.7.2.1, §6.7.3 | `lexer.c`, `parser.c` |
| Compound assignment operators (`*=`, …) | §6.5.16.2 | `preprocessor.c` |
| `__attribute__((…))` GNU extension | (GNU ext, not C99) | `util.c` |
| Incomplete stub `stdio.h` (`putchar`) | — (test stub gap) | `ast_pretty_printer.c` |
