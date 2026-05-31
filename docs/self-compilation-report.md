# Self-Compilation Diagnostic Report

**Date:** 2026-05-31
**Compiler:** `build/ccompiler` (version 00.01.00830000, stage-83)
**Flags:** `--max-errors=0 -Iinclude -Itest/include`

Each `src/*.c` module was compiled independently with the project's own built
compiler. This regeneration follows stage-83 (project source converted to
strict ISO C99). Self-hosting is **not** a stage-83 goal — these results track
the compiler's remaining feature gaps against its own source.

## Summary

| Module | Result | Root cause category |
|---|---|---|
| `ast.c` | FAIL | A — `stdio.h` stub lacks `stderr` |
| `ast_pretty_printer.c` | FAIL | B — array-to-pointer decay of a struct-member array argument |
| `codegen.c` | FAIL | B — 2D array member (via `codegen.h`) |
| `compiler.c` | FAIL | B — 2D array member (via `codegen.h`); cascades into `CodeGen` use |
| `lexer.c` | FAIL | B — `\xNN` hex escape in a character literal |
| `parser.c` | FAIL | B — adjacent string-literal concatenation |
| `preprocessor.c` | FAIL | B — adjacent string-literal concatenation |
| `type.c` | **PASS** | — |
| `util.c` | FAIL | A — `stdio.h` stub lacks `stderr` |
| `version.c` | **PASS** | — |

**Result:** 2 modules passed, 8 failed.

## Changes since the previous report (stage-83 impact)

Stage-83 converted the compiler's own source to strict ISO C99 and, as a side
effect, removed two root causes that previously blocked self-compilation:

- **`__attribute__` gap resolved.** `include/util.h` no longer uses bare
  `__attribute__((noreturn))` / `((format(printf,...)))`; these are now the
  `CC_NORETURN` / `CC_PRINTF` macros, guarded by `#if defined(__GNUC__)`.
  ClaudeC99 does not define `__GNUC__`, so the macros expand to nothing when
  the project is compiled by ClaudeC99 itself. Modules that previously failed
  first on `__attribute__` (`util.c`, `parser.c`, and via headers `codegen.c`,
  `compiler.c`) no longer hit it.
- **Duplicate `ASTNode` typedef resolved.** The redundant
  `typedef struct ASTNode ASTNode;` was deleted from `include/codegen.h`, so
  `codegen.c` and `compiler.c` no longer report the typedef redefinition; their
  first remaining error is the 2D array member at `codegen.h:111`.

As a consequence, `util.c` and `parser.c` now fail on a *different* (and more
fundamental) root cause than before, and `codegen.c`/`compiler.c` fail on a
single header gap instead of three.

---

## Category A — Missing stub system header content

### `stdio.h` stub lacks `stderr`

```
error: undeclared variable 'stderr'
```

**Affected:** `ast.c`, `util.c`.

The `stdio.h` stub in `test/include/` is found and parsed, but it does not
declare the standard streams (`extern FILE *stderr;`, `stdout`, `stdin`). Both
modules call `fprintf(stderr, ...)`, so the first reference to `stderr` is an
undeclared-variable error. This is a stub-content gap, not a compiler bug —
adding the stream declarations to the stub would unblock both modules (modulo
any later feature gaps).

---

## Category B — Language features not yet implemented

### B1 — 2D array struct member

```
include/codegen.h:111:25: error: expected ';', got '[' ('[')
```

Root cause at `include/codegen.h:111`:

```c
char user_labels[MAX_USER_LABELS][MAX_NAME_LEN];
```

The parser does not accept a two-dimensional array declarator on a struct
member; it stops after the first `[...]` and expects `;`. This cascades into
`unknown type name 'CodeGen'` for every later use of the struct, blocking both
`codegen.c` and `compiler.c` (which include `codegen.h`). C99 §6.7.5.2.

**Affected:** `codegen.c:36+` (cascade), `compiler.c:429+` (cascade), all via
`include/codegen.h:111`.

### B2 — `\xNN` hex escape in a character literal

```
error: invalid escape sequence in character literal
```

Root cause at `src/lexer.c:101`:

```c
if (c == '\x01') {
```

The lexer's character-literal escape handler supports the simple escapes
(`\n`, `\t`, …) but not hexadecimal (`\xNN`) escapes. C99 §6.4.4.4.

**Affected:** `lexer.c`.

### B3 — Adjacent string-literal concatenation

```
src/parser.c:1094:37: error: expected ')', got string literal
src/preprocessor.c:603:33: error: expected ')', got string literal
```

Root cause: two string-literal tokens written next to each other are not
concatenated into a single literal. For example, `src/preprocessor.c:601-603`:

```c
fprintf(stderr,
        "error: argument count mismatch for macro '%.*s':"
        " expected %s%d, got %d\n",
        ...);
```

The compiler treats the second literal as an unexpected token rather than
splicing it onto the first. C99 §6.4.5 (translation phase 6).

**Affected:** `parser.c:1094`, `preprocessor.c:603` (each cascades through the
remainder of the enclosing function).

### B4 — Array-to-pointer decay of a struct-member array argument

```
error: function 'operator_name' parameter 'op' expected pointer argument, got integer
```

Root cause at `src/ast_pretty_printer.c:168`:

```c
printf("Binary: %s\n", operator_name(node->value));
```

`node->value` has type `char[MAX_NAME_LEN]` (a struct-member array reached via
`->`), and `operator_name` takes `const char *`. The member array is not
decayed to a pointer in the call-argument position, so the argument check
reports an integer/pointer mismatch. C99 §6.3.2.1.

**Affected:** `ast_pretty_printer.c`.

---

## Successful compilation

- **`type.c`** — uses only already-supported constructs (scalar/pointer types,
  no 2D array members, no adjacent string concatenation, no `stderr`).
- **`version.c`** — small module; `snprintf` into a fixed buffer with supported
  syntax only.

Both compile cleanly to assembly with no errors.

---

## Feature gap summary

| Gap | C99 section | Affected modules |
|---|---|---|
| `stdio.h` stub lacks `stderr`/`stdout`/`stdin` (stub content) | §7.21.1 | `ast.c`, `util.c` |
| 2D array struct member declarator | §6.7.5.2 | `codegen.c`, `compiler.c` (via `codegen.h`) |
| `\xNN` hex escape in character/string literal | §6.4.4.4 | `lexer.c` |
| Adjacent string-literal concatenation | §6.4.5 | `parser.c`, `preprocessor.c` |
| Array-to-pointer decay of a member array argument | §6.3.2.1 | `ast_pretty_printer.c` |

Resolved by stage-83 (no longer appearing): GNU `__attribute__` specifiers and
the duplicate `ASTNode` typedef.
