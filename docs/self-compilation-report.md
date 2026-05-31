# Self-Compilation Diagnostic Report

**Date:** 2026-05-31
**Compiler:** `build/ccompiler`
**Flags:** `--max-errors=0 -Iinclude -Itest/include`

## Summary

| Module | Result | Root cause category |
|---|---|---|
| `ast.c` | FAIL | A — `stderr` missing from `stdio.h` stub |
| `ast_pretty_printer.c` | FAIL | B — member-array does not decay to pointer in call argument |
| `codegen.c` | FAIL | B — duplicate `typedef ASTNode`; `__attribute__` specifier |
| `compiler.c` | FAIL | B — `const` in type-name/struct member; `__attribute__`; duplicate typedef (cascades) |
| `lexer.c` | FAIL | B — `const`-qualified struct member; `\x` hex escape |
| `parser.c` | FAIL | B — `const`-qualified struct member; `__attribute__` (cascades) |
| `preprocessor.c` | FAIL | B — adjacent string-literal concatenation; `\x` hex escape |
| `type.c` | **PASS** | — |
| `util.c` | FAIL | B — `__attribute__` specifier |
| `version.c` | **PASS** | — |

**Result: 2 passed, 8 failed.** All eight failures trace to language-feature
gaps (Category B) except `ast.c`, which is blocked by a single missing symbol
in a stub header (Category A). No `MAX_*` capacity limits were hit, so no
`-D` overrides of `include/constants.h` were required.

---

## Category A — Missing stub system headers

### `stderr` (and `stdout`/`stdin`) not declared in `test/include/stdio.h`

```
src/ast.c: error: undeclared variable 'stderr'
```

`src/ast.c:9` calls `fprintf(stderr, "error: out of memory\n")`, but the
`stdio.h` stub declares the `FILE` type and the `f*` functions only — it never
declares the three standard streams:

```c
/* test/include/stdio.h — present */
typedef struct FILE FILE;
int fprintf(FILE *, const char *, ...);
/* ...but no: extern FILE *stdin, *stdout, *stderr; */
```

This is **not** a compiler bug — the stub is simply incomplete. Adding
`extern FILE *stdin, *stdout, *stderr;` to the stub would unblock `ast.c`.

**Note on transitive impact:** every project module uses `stderr` for its
out-of-memory / diagnostic paths, so this gap would surface in almost every
file. It only appears *first* in `ast.c` because that module includes neither
`util.h`, `lexer.h`, nor `codegen.h`, so no earlier header-level error preempts
it. In the other modules a Category B error fires before the lexer/parser ever
reaches a `stderr` reference.

---

## Category B — Language features not yet implemented

### B1 — `const` type-qualifier rejected in struct members and type-names

```
include/lexer.h:7:5:    error: expected integer type, got 'const'   // const char *source;
src/compiler.c:298:77:  error: expected expression, got 'const'     // sizeof(const char *)
```

The compiler accepts `const` in **function-parameter** declarations
(`operator_name(const char *op)` and every `fprintf(... const char *fmt ...)`
parse fine), but rejects the qualifier in two other positions:

- **Struct member declarations** — `include/lexer.h:7` (`const char *source;`)
  fails with `expected integer type, got 'const'`. This aborts the `Lexer`
  typedef, which is the head of a large cascade (see below).
- **Type-names in `sizeof`/cast operands** — `src/compiler.c:298`
  (`sizeof(const char *)`) fails with `expected expression, got 'const'`.

C99 §6.7.3 permits `const` anywhere a type qualifier may appear. The fix is to
make the declaration-specifier / type-name parser consume `const` (and the
other qualifiers) uniformly, not only in parameter lists.

| File:line | Construct |
|---|---|
| `include/lexer.h:7` | `const char *source;` (struct member) |
| `src/compiler.c:298` | `sizeof(const char *)` (type-name) |

**Cascade:** the `include/lexer.h:7` failure leaves `Lexer` undefined, producing
`unknown type name 'Lexer'` at `lexer.h:19-21`, then `unknown type name 'Parser'`
throughout `parser.h` and `parser.c` (~60 lines), and the duplicate
`switch_depth` "global" reports — all are downstream of this one root cause.

### B2 — `__attribute__` specifier not recognized

```
include/util.h:22:1: error: expected type specifier   // __attribute__((noreturn, format(printf,1,2)))
include/util.h:26:1: error: expected type specifier
include/util.h:31:1: error: expected type specifier
```

`include/util.h` decorates its diagnostic functions with GCC-style
`__attribute__((noreturn, format(...)))`. The parser does not recognize the
`__attribute__` token as a declaration specifier and treats it as a stray
identifier, so it cannot find a type specifier for the declaration that follows.

Because `util.h` is included by `util.c`, `parser.c`, `codegen.c`, and
`compiler.c`, this gap blocks all four. A minimal fix is to lex and discard
`__attribute__((...))` attribute clauses wherever a declaration specifier is
expected.

| File:line | Affected modules |
|---|---|
| `include/util.h:22,26,31` | `util.c`, `parser.c`, `codegen.c`, `compiler.c` |

### B3 — Idempotent/duplicate `typedef` of the same tag not allowed

```
include/codegen.h:85:1: error: duplicate typedef 'ASTNode' in this scope
```

`include/ast.h:62` declares `typedef struct ASTNode { … } ASTNode;` and
`include/codegen.h:85` re-declares the forward form
`typedef struct ASTNode ASTNode;`. Both name the identical type, which C11
§6.7 explicitly permits and which is a common compatibility idiom; C99 strictly
forbids it, so this rejection is technically conformant. Supporting redundant
identical typedefs (the C11 relaxation) would unblock the headers shared by
`codegen.c` and `compiler.c`.

| File:line | Affected modules |
|---|---|
| `include/codegen.h:85` (dup of `include/ast.h:62`) | `codegen.c`, `compiler.c` |

**Cascade:** the failed typedef derails parsing of the anonymous `SwitchCtx`
and `CodeGen` struct bodies, yielding `unknown type name 'SwitchCtx'`,
`unknown type name 'CodeGen'` (~40 lines in `codegen.c`), and the
`int switch_depth;` "duplicate global" report.

### B4 — Struct-member array does not decay to a pointer in a call argument

```
src/ast_pretty_printer.c:
  error: function 'operator_name' parameter 'op' expected pointer argument, got integer
```

`ASTNode.value` is declared `char value[MAX_NAME_LEN]` (`include/ast.h:64`).
The calls `operator_name(node->value)` at `ast_pretty_printer.c:168, 171, 231,
234` pass that member array where a `const char *` is expected. The compiler
fails to apply array-to-pointer decay (C99 §6.3.2.1) to a **member-access**
array expression — it classifies `node->value` as `integer` instead of
`char *`. Plain identifier arrays presumably decay correctly; the gap is
specific to arrays reached through `.`/`->`.

| File:line | Construct |
|---|---|
| `src/ast_pretty_printer.c:168,171,231,234` | `operator_name(node->value)` |

### B5 — `\xHH` hex escape sequences not recognized

```
src/lexer.c:        error: invalid escape sequence in character literal   // '\x01'
src/preprocessor.c: error: invalid escape sequence in string literal      // "\x01"
```

The compiler's own lexer does not implement hexadecimal escape sequences
(C99 §6.4.4.4). It rejects them in both character literals
(`src/lexer.c:100` `'\x01'`) and string literals (`src/preprocessor.c:1380,1391`
`"\x01" …`). Decimal/octal/simple escapes (`\0`, `\n`, `\\`, `\'`) are handled;
only the `\x` form is missing.

| File:line | Construct |
|---|---|
| `src/lexer.c:100` | `'\x01'` (char literal) |
| `src/preprocessor.c:1380,1391` | `"\x01"` (string literal) |

### B6 — Adjacent string-literal concatenation not supported

```
src/preprocessor.c:602:33: error: expected ')', got string literal
        (' expected %s%d, got %d\n')
```

`src/preprocessor.c:602-603` (and `:1380`) rely on translation-phase-6 string
concatenation:

```c
fprintf(stderr,
        "error: argument count mismatch for macro '%.*s':"
        " expected %s%d, got %d\n",
        ...);
```

After parsing the first string literal the compiler expects `)` or `,` and
errors on encountering the second adjacent literal. Implementing adjacent
string-literal concatenation (C99 §6.4.5) would resolve this.

| File:line | Construct |
|---|---|
| `src/preprocessor.c:602-603` | `"…:" " expected %s%d…"` |
| `src/preprocessor.c:1380` | `"\x01" "1:%s\n"` |

---

## Successful compilation

| Module | Why it compiled |
|---|---|
| `type.c` | `compiled: src/type.c -> type.asm`. Uses only features already supported; includes no header carrying a `const` struct member or `__attribute__`, and its source avoids hex escapes, adjacent string concatenation, and member-array call arguments. |
| `version.c` | `compiled: src/version.c -> version.asm`. Small module with no exposure to any of the six feature gaps above. |

---

## Feature gap summary

| Gap | C99 section | Affected modules |
|---|---|---|
| `const` qualifier in struct members / type-names | §6.7.3 | `lexer.c`, `parser.c`, `compiler.c`, `codegen.c` (via headers) |
| `__attribute__` specifier ignored/parsed | (GCC ext.) | `util.c`, `parser.c`, `codegen.c`, `compiler.c` |
| Redundant identical `typedef` (C11 idiom) | C11 §6.7 (C99 forbids) | `codegen.c`, `compiler.c` |
| Member-array → pointer decay in call arg | §6.3.2.1 | `ast_pretty_printer.c` |
| `\xHH` hex escape sequences | §6.4.4.4 / §6.4.5 | `lexer.c`, `preprocessor.c` |
| Adjacent string-literal concatenation | §6.4.5 | `preprocessor.c` |
| (Stub) `stderr`/`stdout`/`stdin` undeclared | — | `ast.c` (+ all modules functionally) |

### Capacity limits

No `MAX_*` capacity errors (e.g. `MAX_FUNCTIONS`, `MAX_LOCALS`) were
encountered, so no `-D` overrides of `include/constants.h` were necessary for
this pass.
