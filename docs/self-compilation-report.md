# Self-Compilation Diagnostic Report

**Date:** 2026-05-31
**Compiler:** `build/ccompiler`
**Flags:** `--max-errors=0 -Iinclude -Itest/include`

## Summary

| Module | Result | Root cause category |
|---|---|---|
| `ast.c` | FAIL | A — `stdlib.h` stub missing `calloc` |
| `ast_pretty_printer.c` | FAIL | A — `stdio.h` stub missing `putchar` |
| `codegen.c` | FAIL | B — idempotent typedef redefinition, multidimensional array member, `__attribute__`; capacity limit (max 64 functions) |
| `compiler.c` | FAIL | B — cascades from `codegen.h`/`lexer.h`/`parser.h`/`util.h` (typedef redef, `const` member, `__attribute__`) |
| `lexer.c` | FAIL | B — `const` struct member (`lexer.h`), `\x` hex escape |
| `parser.c` | FAIL | B — `const` struct member (`lexer.h`), `__attribute__` (`util.h`) |
| `preprocessor.c` | FAIL | B — adjacent string-literal concatenation, `\x` hex escape; capacity limit (max 64 functions) |
| `type.c` | PASS | — |
| `util.c` | FAIL | B — `__attribute__` specifier (`util.h` + definitions) |
| `version.c` | PASS | — |

**8 modules failed, 2 passed.**

---

## Category A — Missing / incomplete stub system headers

These are not compiler bugs. The header exists in `test/include/`, but the
particular library function being called has no declaration in the stub, so the
C99 rule against calling an undeclared function (no implicit declarations)
fires correctly. The fix is to extend the stub.

### `stdlib.h` — missing `calloc`

```
src/ast.c: error: call to undefined function 'calloc'
```

The current stub declares only `malloc`, `realloc`, and `free`
(`test/include/stdlib.h:7-9`). `src/ast.c` uses `calloc`, which is undeclared.

| File:line | Call |
|---|---|
| `src/ast.c` (first use) | `calloc(...)` |

### `stdio.h` — missing `putchar`

```
src/ast_pretty_printer.c: error: call to undefined function 'putchar'
```

The stub declares `printf`, `fprintf`, `snprintf`, the `v*printf` family, etc.,
but not `putchar`. `src/ast_pretty_printer.c` calls `putchar`.

| File:line | Call |
|---|---|
| `src/ast_pretty_printer.c` (first use) | `putchar(...)` |

> Note: once the stubs are extended these two modules should be revisited — each
> error above is the *first* undefined-function diagnostic and the compiler
> stops the pass at it, so further gaps may be hidden behind it.

---

## Category B — Language features not yet implemented

Each gap below was reproduced in isolation against `build/ccompiler` to confirm
it is a genuine front-end rejection of valid C and not a cascade artifact.

### B1 — `const` (type qualifier) in a declaration

```
include/lexer.h:7:5: error: expected integer type, got 'const'
```

The struct member `const char *source;` is rejected; the type-qualifier keyword
`const` is not accepted where a type specifier is expected. Because this breaks
the `Lexer` typedef, every subsequent `Lexer` reference cascades into
`unknown type name 'Lexer'`.

Isolated repro: `struct S { const char *p; };` →
`error: expected integer type, got 'const'`.

C99 §6.7.3 (type qualifiers).

| File:line | Construct | Affected modules |
|---|---|---|
| `include/lexer.h:7` | `const char *source;` | `lexer.c`, `parser.c`, `compiler.c` |

### B2 — `__attribute__` specifier

```
include/util.h:22:1: error: expected type specifier
src/util.c:14:1:    error: expected type specifier
```

`__attribute__((...))` decorators (e.g. `noreturn`, `format(printf, …)`) are not
recognized; the parser sees an unexpected token where a type specifier is
expected and the following declaration/definition fails.

Isolated repro: `__attribute__((noreturn)) void f(void);` →
`error: expected type specifier`.

GCC extension (used throughout the codebase; not strict C99).

| File:line | Construct | Affected modules |
|---|---|---|
| `include/util.h:22,26,31` | function-decl attributes | `util.c`, `parser.c`, `compiler.c`, `codegen.c` |
| `src/util.c:14,28,37` | function-def attributes | `util.c` |

### B3 — Idempotent (redundant) typedef redefinition

```
include/codegen.h:94:1: error: duplicate typedef 'ASTNode' in this scope
```

`typedef struct ASTNode ASTNode;` is rejected because `ASTNode` was already
typedef'd (via `ast.h`). Redefining a typedef to the *same* type is permitted in
C11 §6.7/3 but is a constraint violation in C99; the codebase relies on the C11
behavior. After this error the rest of `codegen.h` mis-parses (`SwitchCtx`,
`CodeGen` cascades).

Isolated repro: `typedef struct N N; typedef struct N N;` →
`error: duplicate typedef 'N' in this scope`.

C11 §6.7/3 (used as an extension over C99).

| File:line | Construct | Affected modules |
|---|---|---|
| `include/codegen.h:94` | `typedef struct ASTNode ASTNode;` | `codegen.c`, `compiler.c` |

### B4 — Multidimensional array declarator

```
include/codegen.h:121:25: error: expected ';', got '[' ('[')
```

The member `char user_labels[MAX_USER_LABELS][256];` is rejected at the *second*
`[`: the declarator parser handles one array dimension but not a second.

Isolated repro: `struct S { char m[4][8]; };` →
`error: expected ';', got '[' ('[')`.

C99 §6.7.5.2 (array declarators).

| File:line | Construct | Affected modules |
|---|---|---|
| `include/codegen.h:121` | `char user_labels[...][256];` | `codegen.c`, `compiler.c` |

### B5 — Adjacent string-literal concatenation

```
src/preprocessor.c:604:33: error: expected ')', got string literal
```

Two adjacent string literals (here a multi-line `fprintf` format split across
source lines) are not concatenated into one; after the first literal the parser
expects the closing `)` and trips on the second literal.

Isolated repro: `char *p = "abc" "def";` →
`error: expected ';', got string literal ('def')`.

C99 §6.4.5 (string literal concatenation).

| File:line | Construct | Affected modules |
|---|---|---|
| `src/preprocessor.c:604` | split `fprintf` format string | `preprocessor.c` |

### B6 — `\x` hexadecimal escape sequence

```
src/lexer.c:        error: invalid escape sequence in character literal
src/preprocessor.c: error: invalid escape sequence in string literal
```

The hex escape `\x01` (used for the preprocessor's enter/return file markers) is
rejected in both character and string literals.

Isolated repro: `char c = '\x01';` →
`error: invalid escape sequence in character literal`.

C99 §6.4.4.4 (character constants / hexadecimal-escape-sequence).

| File:line | Construct | Affected modules |
|---|---|---|
| `src/lexer.c:100` | `c == '\x01'` | `lexer.c` |
| `src/preprocessor.c:1382,1393` | `"\x01" "..."` | `preprocessor.c` |

### B7 — Compiler capacity limit: `too many functions (max 64)`

```
src/codegen.c:983:70:      error: too many functions (max 64)
src/preprocessor.c:1068:70: error: too many functions (max 64)
```

Not a C99 feature gap but a fixed front-end table size: the compiler caps a
translation unit at 64 functions. `codegen.c` and `preprocessor.c` exceed it.
This limit must be raised (or made dynamic) for self-compilation of the larger
modules even after B1–B6 are fixed.

| File:line | Affected modules |
|---|---|
| `src/codegen.c:983, 1003, 3935, 3949, 3995, 4089` | `codegen.c` |
| `src/preprocessor.c:1068` | `preprocessor.c` |

---

## Successful compilation

| Module | Why it succeeded |
|---|---|
| `type.c` | Includes only `stddef.h`, `stdio.h`, `stdlib.h`, and `type.h`. None of those headers use `const` members, `__attribute__`, redundant typedefs, or multidimensional members, and the module itself stays within the 64-function cap and uses no `\x` escapes or adjacent-literal concatenation. |
| `version.c` | Trivial module including only `version.h` and `stdio.h`; exercises none of the gap features. |

---

## Feature gap summary

| Gap | C99/C11 section | Affected modules |
|---|---|---|
| A: `calloc` missing from `stdlib.h` stub | — (stub) | `ast.c` |
| A: `putchar` missing from `stdio.h` stub | — (stub) | `ast_pretty_printer.c` |
| B1: `const` type qualifier in declarations | C99 §6.7.3 | `lexer.c`, `parser.c`, `compiler.c` |
| B2: `__attribute__` specifier | GCC ext. | `util.c`, `parser.c`, `compiler.c`, `codegen.c` |
| B3: idempotent typedef redefinition | C11 §6.7/3 | `codegen.c`, `compiler.c` |
| B4: multidimensional array declarator | C99 §6.7.5.2 | `codegen.c`, `compiler.c` |
| B5: adjacent string-literal concatenation | C99 §6.4.5 | `preprocessor.c` |
| B6: `\x` hexadecimal escape sequence | C99 §6.4.4.4 | `lexer.c`, `preprocessor.c` |
| B7: capacity limit — max 64 functions/TU | — (limit) | `codegen.c`, `preprocessor.c` |
