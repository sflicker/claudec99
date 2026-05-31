# Self-Compilation Diagnostic Report

**Date:** 2026-05-31
**Compiler:** `build/ccompiler`
**Flags:** `--max-errors=0 -Iinclude -Itest/include`

## Summary

| Module | Result | Root cause category |
|---|---|---|
| `ast.c` | FAIL | A — `stdio.h` stub lacks `stderr` |
| `ast_pretty_printer.c` | FAIL | B — array-to-pointer decay of a struct member array argument |
| `codegen.c` | FAIL | B — `__attribute__`, duplicate typedef, 2D array member (all via headers) |
| `compiler.c` | FAIL | B — same header gaps as `codegen.c` (cascades into `CodeGen` use) |
| `lexer.c` | FAIL | B — `\xNN` hex escape in a character literal |
| `parser.c` | FAIL | B — `__attribute__` (via `util.h`) + adjacent string-literal concatenation |
| `preprocessor.c` | FAIL | B — adjacent string-literal concatenation |
| `type.c` | **PASS** | — |
| `util.c` | FAIL | B — `__attribute__` (via `util.h`) |
| `version.c` | **PASS** | — |

2 modules passed, 8 failed. No `MAX_*` limits were hit, so no `-D` overrides were needed.

---

## Category A — Missing / incomplete stub system headers

### `stdio.h` — missing `stderr` (and the standard streams)

```
src/ast.c: error: undeclared variable 'stderr'
```

`test/include/stdio.h` declares the `printf`/`fprintf` family but does **not**
declare the standard stream objects. `grep -n "stderr\|stdout\|stdin"
test/include/stdio.h` returns nothing.

`ast.c` is the only module whose *first* error is this gap (it includes
`<stdio.h>` but not `util.h`, so it parses past the header into the body and
then references `stderr` directly). Many other modules also use `stderr`, but
they fail earlier on the Category B gaps below.

**Fix:** add the conventional declarations to the stub, e.g.

```c
extern FILE *stdin;
extern FILE *stdout;
extern FILE *stderr;
```

This is a stub-completeness gap, **not** a compiler bug.

---

## Category B — Language features not yet implemented

### B1 — GNU `__attribute__((...))` specifier

```
include/util.h:22:1: error: expected type specifier
include/util.h:26:1: error: expected type specifier
include/util.h:31:1: error: expected type specifier
```

`include/util.h` precedes three function declarations with GNU attributes:

```c
__attribute__((noreturn, format(printf, 1, 2)))
void compile_error(const char *fmt, ...);
```

The compiler does not recognize `__attribute__`, so the attribute line is
consumed as a stray construct and the following declaration line trips
"expected type specifier". (Lines 22/26/31 are the declaration lines *after*
the attributes at 21/25/30.)

`__attribute__` is a GNU extension, not strict C99, but because `util.h` is
included widely it blocks every module that pulls it in.

**Affected locations (transitively, via `include/util.h`):**

| File:line | Declaration |
|---|---|
| `include/util.h:22` | `void compile_error(...)` |
| `include/util.h:26` | `void compile_error_at(...)` |
| `include/util.h:31` | `void compile_warning_at(...)` |

Blocks `parser.c`, `util.c`, `codegen.c`, and `compiler.c`.

### B2 — Redefinition of an identical typedef (C11-legal, C99-illegal)

```
include/codegen.h:85:1: error: duplicate typedef 'ASTNode' in this scope
```

`include/ast.h:62` already provides `typedef struct ASTNode { … } ASTNode;`.
`include/codegen.h:84` (which `#include`s `ast.h`) repeats the forward form
`typedef struct ASTNode ASTNode;`. C11 §6.7¶3 permits an identical typedef
redefinition; C99 forbids it. The compiler is correctly strict for C99, but the
project source relies on the C11 relaxation.

The rejection cascades: with `ASTNode` not installed, `codegen.h:90`
(`ASTNode *nodes[…]`) and every downstream `CodeGen`/`SwitchCtx` use report
"unknown type name".

**Fix options:** accept identical-type typedef redefinition (C11 behaviour), or
drop the redundant typedef from `codegen.h`.

Blocks `codegen.c` and `compiler.c`.

### B3 — Multidimensional (2D) array member declaration

```
include/codegen.h:112:25: error: expected ';', got '[' ('[')
```

The `CodeGen` struct declares a 2-D array member:

```c
char user_labels[MAX_USER_LABELS][MAX_NAME_LEN];
```

The compiler parses the first `[…]` array suffix on a member but rejects the
second, so multidimensional arrays (C99 §6.7.5.2 / §6.5.2.1) are unsupported in
member declarations.

| File:line | Declaration |
|---|---|
| `include/codegen.h:112` | `char user_labels[MAX_USER_LABELS][MAX_NAME_LEN];` |

Blocks `codegen.c` and `compiler.c`.

### B4 — Adjacent string-literal concatenation (C99 §6.4.5)

```
src/parser.c:1094:37: error: expected ')', got string literal (' type (max %d)\n')
src/preprocessor.c:602:33: error: expected ')', got string literal (' expected %s%d, got %d\n')
```

C99 requires adjacent string literals to be concatenated into one literal
during translation phase 6. The compiler treats the second literal as an
unexpected token, expecting `)` or `,` after the first.

| File:line | Source |
|---|---|
| `src/parser.c:1137-1138` | `"error: too many parameters in function pointer"` `" type (max %d)\n"` |
| `src/preprocessor.c:601-602` | `"error: argument count mismatch for macro '%.*s':"` `" expected %s%d, got %d\n"` |

Blocks `parser.c` and `preprocessor.c`.

### B5 — `\xNN` hex escape in a character literal

```
src/lexer.c: error: invalid escape sequence in character literal
```

The lexer's own character-escape handler (`src/lexer.c:311-327`) supports
`a b f n r t v \\ ' " ? 0` but **not** the `\x` hex escape form. `lexer.c`
itself uses `'\x01'` (e.g. `src/lexer.c:100`, `if (c == '\x01')`) as an
`#include`-boundary sentinel, so the compiler rejects its own source. The
string-literal escape handler (`src/lexer.c:252-263`) is likewise missing `\x`.

| File:line | Literal |
|---|---|
| `src/lexer.c:100` | `'\x01'` |

This is C99 §6.4.4.4 (hexadecimal-escape-sequence). Blocks `lexer.c`.

### B6 — Array-to-pointer decay of a struct member array passed as an argument

```
src/ast_pretty_printer.c: error: function 'operator_name' parameter 'op'
    expected pointer argument, got integer
```

`operator_name(const char *op)` is called as `operator_name(node->value)`
(e.g. `src/ast_pretty_printer.c:168`). `node->value` is the member
`char value[MAX_NAME_LEN]` (`include/ast.h:63`). When a member-array expression
is used as a call argument it must decay to `char *` (C99 §6.3.2.1¶3). The
compiler instead types the member array as an integer, so the pointer-parameter
check fails.

| File:line | Call |
|---|---|
| `src/ast_pretty_printer.c:168` | `operator_name(node->value)` |
| `src/ast_pretty_printer.c:171` | `operator_name(node->value)` |
| `src/ast_pretty_printer.c:231` | `operator_name(node->value)` |
| `src/ast_pretty_printer.c:234` | `operator_name(node->value)` |

Blocks `ast_pretty_printer.c`.

---

## Successful compilation

| Module | Why it compiled |
|---|---|
| `type.c` | Includes only `type.h`/`ast.h` and uses no `__attribute__`, adjacent-string, 2-D array, member-array-decay, or `\x`-escape constructs. |
| `version.c` | Tiny module with no problematic headers or constructs. |

Both produced assembly output (`type.asm`, `version.asm`).

---

## Feature gap summary

| Gap | C99 / standard section | Affected modules |
|---|---|---|
| GNU `__attribute__` specifier | GNU extension (not C99) | `parser.c`, `util.c`, `codegen.c`, `compiler.c` |
| Identical typedef redefinition | C11 §6.7¶3 (C99 forbids) | `codegen.c`, `compiler.c` |
| Multidimensional array member | §6.7.5.2 / §6.5.2.1 | `codegen.c`, `compiler.c` |
| Adjacent string-literal concatenation | §6.4.5 | `parser.c`, `preprocessor.c` |
| `\xNN` hex escape in literals | §6.4.4.4 | `lexer.c` |
| Member-array → pointer decay as argument | §6.3.2.1¶3 | `ast_pretty_printer.c` |
| Incomplete stub: `stdio.h` standard streams | (stub gap, not a feature) | `ast.c` |
