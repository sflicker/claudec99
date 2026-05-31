# Self-Compilation Diagnostic Report

**Date:** 2026-05-31
**Compiler:** `build/ccompiler`
**Flags:** `--max-errors=0 -Iinclude -Itest/include`

## Summary

| Module | Result | Root cause category |
|---|---|---|
| `ast.c` | FAIL | A — `calloc` not declared in `stdlib.h` stub |
| `ast_pretty_printer.c` | FAIL | A — `putchar` not declared in `stdio.h` stub |
| `codegen.c` | FAIL | B — redundant `ASTNode` typedef; `__attribute__`; `MAX_FUNCTIONS` limit |
| `compiler.c` | FAIL | B — redundant typedef; `const` member; `__attribute__`; adjacent string literals; member-name collision |
| `lexer.c` | FAIL | B — `const`-qualified struct member; `\xNN` hex escape |
| `parser.c` | FAIL | B — `const`-qualified struct member; `__attribute__` |
| `preprocessor.c` | FAIL | B — adjacent string-literal concatenation; `\xNN` hex escape; `MAX_FUNCTIONS` limit |
| `type.c` | **PASS** | — |
| `util.c` | FAIL | B — `__attribute__` |
| `version.c` | **PASS** | — |

**Result: 2 passed, 8 failed.**

---

## Category A — Missing stub system header declarations

No file hit a literal `include file not found` error — every required header has
a stub in `test/include/`. However two files are blocked by **functions missing
from existing stubs**. The header parses fine, but a referenced library function
has no declaration, so the call is rejected with `call to undefined function`.

### `calloc` — missing from `test/include/stdlib.h`

```
src/ast.c: error: call to undefined function 'calloc'
```

`test/include/stdlib.h` declares `malloc`, `realloc`, and `free`, but **not**
`calloc`. This is the only error in `ast.c`; adding

```c
void *calloc(size_t, size_t);
```

to the stub would unblock it.

**Affected:** `ast.c`.

### `putchar` — missing from `test/include/stdio.h`

```
src/ast_pretty_printer.c: error: call to undefined function 'putchar'
```

`test/include/stdio.h` has no `putchar` (nor `putc` / `fputc`) declaration. This
is the only error in `ast_pretty_printer.c`. Adding

```c
int putchar(int);
```

to the stub would unblock it.

**Affected:** `ast_pretty_printer.c`.

---

## Category B — Language features not yet implemented

### B1 — `const`-qualified declarations

A `const` qualifier in a declaration is rejected. The first member of `Lexer`
is `const char *source;` and the compiler reports:

```
include/lexer.h:7:5: error: expected integer type, got 'const'
```

Minimal repro:

```c
struct S { const char *p; int x; };   // error: expected integer type, got 'const'
```

Because the `Lexer` typedef fails to parse, every later use of `Lexer` cascades
into `unknown type name 'Lexer'`. C99 §6.7.3 (type qualifiers).

| File:line | Context |
|---|---|
| `include/lexer.h:7` | `const char *source;` (root) |
| `src/lexer.c:11,33,44,…` | cascade: `unknown type name 'Lexer'` |
| `src/parser.c:48,64,…` | cascade via `parser.h` → `lexer.h` |
| `src/compiler.c` | cascade via `lexer.h` |

### B2 — `__attribute__((...))` GNU extension

`util.h` decorates its error/warning prototypes with GNU attributes. The
compiler does not recognize `__attribute__`:

```
include/util.h:22:1: error: expected type specifier
```

Minimal repro:

```c
__attribute__((noreturn)) void f(void);   // error: expected type specifier
```

Not part of C99 (GNU extension), but used pervasively in `util.h`.

| File:line | Context |
|---|---|
| `include/util.h:22,26,31` | `__attribute__((noreturn, format(...)))` (root) |
| `src/util.c:14,28,37` | the definitions of those functions |
| `parser.c`, `compiler.c`, `codegen.c` | cascade via `util.h` |

### B3 — Redundant `typedef` redeclaration

`ast.h` defines `typedef struct ASTNode { … } ASTNode;` and `codegen.h` repeats
the forward form `typedef struct ASTNode ASTNode;`. The compiler treats the
second as a clash:

```
include/codegen.h:85:1: error: duplicate typedef 'ASTNode' in this scope
```

Minimal repro:

```c
typedef struct N { int x; } N;
typedef struct N N;        // error: duplicate typedef 'N' in this scope
```

C99 §6.7 technically forbids redeclaring a typedef in the same scope; C11
§6.7p3 explicitly permits it when the definitions are identical. Most compilers
accept the C11 behaviour. The failed typedef cascades into `unknown type name
'CodeGen'` / `'SwitchCtx'` throughout `codegen.c`.

| File:line | Context |
|---|---|
| `include/codegen.h:85` | `typedef struct ASTNode ASTNode;` (root) |
| `src/codegen.c` | cascade: `unknown type name 'CodeGen'` (×30+) |
| `src/compiler.c` | cascade via `codegen.h` |

### B4 — Adjacent string-literal concatenation

C99 string literals written next to each other are concatenated into one. The
compiler does not do this:

```
src/preprocessor.c:602:33: error: expected ')', got string literal
```

at the `fprintf` whose format string is split across lines:

```c
fprintf(stderr,
        "error: argument count mismatch for macro '%.*s':"
        " expected %s%d, got %d\n",      // <- second literal not joined
        ...);
```

Minimal repro:

```c
char *p = "abc" "def";   // error: expected ';', got string literal
```

C99 §6.4.5p5 (adjacent string literals are concatenated).

| File:line | Context |
|---|---|
| `src/preprocessor.c:602` | split `fprintf` format string (root) |
| `src/compiler.c:298` | `expected expression` from a split literal |

### B5 — Hex escape sequences `\xNN`

Hexadecimal escapes in character and string literals are rejected:

```
src/lexer.c: error: invalid escape sequence in character literal
src/preprocessor.c: error: invalid escape sequence in string literal
```

Minimal repro:

```c
int c = '\x01';   // error: invalid escape sequence in character literal
```

`lexer.c` uses `'\x01'` as the include-marker sentinel; `preprocessor.c` emits
`"\x01" "…"` markers. C99 §6.4.4.4 (hexadecimal-escape-sequence).

| File:line | Context |
|---|---|
| `src/lexer.c:67,100` | `c == '\x01'` (include marker) |
| `src/preprocessor.c:1380,1391` | `"\x01" "…"` enter/return-file markers |

### B6 — Struct member names share a global namespace

Two distinct structs each declare a member named `switch_depth`:

- `Parser` (`include/parser.h:74`)
- `CodeGen` (`include/codegen.h:107`)

When both headers are in scope (as in `compiler.c`), the compiler reports:

```
include/parser.h:74:21: error: duplicate global declaration 'switch_depth'
```

This is a genuine compiler limitation: per C99 §6.2.3, members of each structure
or union have a **separate name space** keyed by the aggregate. The compiler is
instead tracking member identifiers in a single flat namespace, so a member name
reused in a different struct collides. (Only `switch_depth` is flagged because it
is the only member name common to both structs.)

| File:line | Context |
|---|---|
| `include/parser.h:74` | `int switch_depth;` inside `Parser` |
| `include/codegen.h:107` | `int switch_depth;` inside `CodeGen` |
| `src/compiler.c` | error surfaces here (both headers included) |

---

## Compiler limits encountered

### `PARSER_MAX_FUNCTIONS` (default 64)

`codegen.c` and `preprocessor.c` exceed the 64-function signature table:

```
src/codegen.c:983: error: too many functions (max 64)
src/preprocessor.c:1066: error: too many functions (max 64)
```

**How the limit actually works.** The "max 64" capacity is *baked into the
`ccompiler` binary itself* — it is the size of the `FuncSig funcs[...]` table in
the `Parser` struct, fixed when `ccompiler` was compiled. Passing
`-DPARSER_MAX_FUNCTIONS=256` *to `ccompiler` while it compiles a target `.c`
file* therefore **cannot** raise it: that `-D` only defines a macro in the
*target's* preprocessing context, not in `ccompiler`'s own already-compiled code.

> Note: an earlier draft of this report claimed `-DPARSER_MAX_FUNCTIONS=256`
> "dropped the count to 0." That was a misread — at the time `constants.h`
> defined the macro unconditionally, so the `-D` collided with it and produced a
> fatal `macro 'PARSER_MAX_FUNCTIONS' redefined with incompatible replacement`
> error that aborted the compile *before* the function-count check ran. The
> limit was never actually raised.

**Correct workaround — rebuild `ccompiler` with a larger table.**
`include/constants.h` now wraps every limit in `#ifndef` guards, so the table
size can be overridden cleanly at `ccompiler`'s **own build time**:

```sh
cmake -B build -DCMAKE_C_FLAGS="-DPARSER_MAX_FUNCTIONS=256 -DPARSER_MAX_GLOBALS=256 \
    -DMAX_GLOBALS=256 -DMAX_LOCALS=256"
cmake --build build
```

This rebuilds with no `redefined` diagnostic, and the resulting compiler
self-compiles `codegen.c` and `preprocessor.c` with **zero** `too many functions`
errors (verified).

**No new feature gaps revealed.** Recompiling both files with the raised limits
surfaced no new error categories — only the root causes already catalogued above
remain:

- `codegen.c`: the B3 duplicate-`ASTNode`-typedef cascade (`unknown type name
  'CodeGen'` / `'SwitchCtx'`) plus the B2 `__attribute__` errors from `util.h`.
- `preprocessor.c`: the B4 adjacent-string-literal and B5 `\xNN`-escape errors,
  whose parse-recovery fallout produces the `expected type specifier` and
  `non-constant initializer for global` cascades.

In other words, the function-count limit was not masking any additional
unimplemented feature.

---

## Successful compilation

| Module | Why it compiled |
|---|---|
| `type.c` | Includes only `stddef.h`, `stdio.h`, `stdlib.h`, and `type.h`. Uses no `calloc`/`putchar`, no `const` members, no `__attribute__`, no split string literals or `\x` escapes; under the function limit. |
| `version.c` | Includes only `version.h` and `stdio.h`; tiny module with none of the unsupported constructs. |

Notably, both passing files avoid every Category-B construct *and* depend only on
headers (`type.h`, `version.h`) that do not pull in `lexer.h`/`util.h`/`codegen.h`
(the headers carrying the `const` members, `__attribute__`s, and redundant
typedefs that block everything else).

---

## Feature gap summary

| Gap | C99 section | Affected modules |
|---|---|---|
| `const`-qualified declarations | §6.7.3 | `lexer.c`, `parser.c`, `compiler.c` |
| `__attribute__((...))` (GNU ext.) | n/a (extension) | `util.c`, `parser.c`, `compiler.c`, `codegen.c` |
| Redundant identical `typedef` | §6.7 / C11 §6.7p3 | `codegen.c`, `compiler.c` |
| Adjacent string-literal concatenation | §6.4.5 | `preprocessor.c`, `compiler.c` |
| Hex escape sequences `\xNN` | §6.4.4.4 | `lexer.c`, `preprocessor.c` |
| Per-struct member name spaces | §6.2.3 | `compiler.c` |
| Missing stub decls (`calloc`, `putchar`) | n/a (stub gap) | `ast.c`, `ast_pretty_printer.c` |
| `PARSER_MAX_FUNCTIONS` limit (64) | n/a (limit) | `codegen.c`, `preprocessor.c` |
