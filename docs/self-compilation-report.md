# Self-Compilation Diagnostic Report

**Date:** 2026-05-25
**Compiler:** `build/ccompiler`
**Flags:** `--max-errors=0 -Iinclude -Itest/include`

## Summary

| Module | Result | Root cause category |
|---|---|---|
| `ast.c` | FAIL | B — subscript of member-access; for-init declaration |
| `ast_pretty_printer.c` | FAIL | B — for-init declaration; enum case labels |
| `codegen.c` | FAIL | A — missing `<stdarg.h>` |
| `compiler.c` | FAIL | A — missing `<stdarg.h>` |
| `lexer.c` | FAIL | B — `const` in struct member; hex escape sequences |
| `parser.c` | FAIL | A — missing `<stdarg.h>` |
| `preprocessor.c` | FAIL | B — compound assignment; for-init; subscript; hex escape; adjacent string literals; postfix `++` on non-identifier |
| `type.c` | FAIL | B — for-init declaration; enum case labels |
| `util.c` | FAIL | A — missing `<stdarg.h>` |
| `version.c` | PASS | — |

---

## Category A — Missing stub system headers

### `<stdarg.h>`

Four modules fail immediately with `error: include file not found: <stdarg.h>`.
No stub exists under `test/include/`.  All other errors in these files are
unseen (compilation aborts on the first missing include).

**Affected modules:** `codegen.c`, `compiler.c`, `parser.c`, `util.c`

---

## Category B — Language features not yet implemented

### B-1 — For-init declarations (`for (type var = …; …; …)`)

C99 §6.8.5.3 permits a declaration in the `for` initializer clause.  The
parser expects an expression, not a type specifier, and fails with:

```
error: expected expression, got 'int'
```

The first occurrence in each file drives a cascade of `expected type
specifier` errors for the remainder of the function body.

| File | Line | Construct |
|---|---|---|
| `src/ast.c` | 27 | `for (int i = 0; i < node->child_count; i++)` |
| `src/ast_pretty_printer.c` | 29 | `for (int i = 0; i < node->child_count; i++)` |
| `src/type.c` | 95 | `for (int i = 0; i < …; i++)` |
| `src/preprocessor.c` | 91 | `for (size_t i = 0; …)` |
| `src/preprocessor.c` | 104 | `for (size_t i = 0; …)` |
| `src/preprocessor.c` | 124 | `for (int i = 0; …)` |
| `src/preprocessor.c` | 162 | `for (size_t i = 0; …)` |
| `src/preprocessor.c` | 233 | `for (int i = 0; …)` |
| `src/preprocessor.c` | 268 | `for (int i = 0; …)` |
| `src/preprocessor.c` | 412 | `for (size_t i = 0; …)` |

### B-2 — Enum/identifier `case` labels in `switch`

C99 §6.8.4.2 allows any integer constant expression as a `case` label,
including named enum constants.  The compiler only accepts integer literals:

```
error: expected integer literal, got identifier ('AST_TRANSLATION_UNIT')
```

| File | Line | Identifier |
|---|---|---|
| `src/ast_pretty_printer.c` | 69 | `AST_TRANSLATION_UNIT` |
| `src/type.c` | 147 | `TYPE_VOID` |
| `src/type.c` | 175 | `TYPE_BOOL` |

### B-3 — Subscript of a member-access (arrow) expression

C99 §6.5.2.1 allows any expression as the subscript base.  The compiler
requires a plain identifier:

```
error: subscript base must be an identifier
```

| File | Line | Construct |
|---|---|---|
| `src/ast.c` | 21 | `parent->children[parent->child_count++]` |
| `src/preprocessor.c` | 353 | struct-pointer member subscript (`s->defs[i]`) |
| `src/preprocessor.c` | 485 | same pattern |

### B-4 — Compound assignment operators (`*=`, `+=`, …)

The parser does not recognise compound assignment operators.  After parsing
the left-hand side it expects `;`:

```
error: expected ';', got '*=' ('*=')
error: expected ';', got '+=' ('+=')
```

The two root-cause sites in `preprocessor.c` are inside `gbuf_push` and
`gbuf_append`, which are called throughout the file.  Their parse failure
creates a large cascade of `expected type specifier` errors that accounts
for the majority of the 200+ errors reported for `preprocessor.c`.

| File | Line | Operator |
|---|---|---|
| `src/preprocessor.c` | 36 | `g->cap *= 2;` |
| `src/preprocessor.c` | 50 | `g->cap += …;` |

### B-5 — Hex escape sequences (`\x…`) in character and string literals

C99 §6.4.4.4 defines `\x` hexadecimal escape sequences.  The compiler
rejects them:

```
error: invalid escape sequence in character literal
error: invalid escape sequence in string literal
```

| File | Line | Literal |
|---|---|---|
| `src/lexer.c` | 100 | `'\x01'` |
| `src/preprocessor.c` | 1382 | `"\x01"` |

### B-6 — Adjacent string literal concatenation

C99 §6.4.5 requires adjacent string literals to be concatenated during
translation.  The compiler treats the second literal as a spurious token:

```
error: expected ')', got string literal (' expected %s%d, got %d\n')
```

| File | Line | Construct |
|---|---|---|
| `src/preprocessor.c` | 1382 | `"\x01" "1:%s\n"` |

Note: this site also triggers B-5 (`\x01`); both gaps apply simultaneously.

### B-7 — Postfix `++`/`--` on non-identifier lvalues

C99 §6.5.2.4 allows postfix increment/decrement on any modifiable lvalue.
The compiler requires a plain identifier:

```
error: postfix ++ requires an identifier
```

The pattern `(*ptr)++` appears repeatedly in `preprocessor.c`'s
`eval_cond_*` family of functions:

| File | Line | Construct |
|---|---|---|
| `src/preprocessor.c` | 735 | `(*in)++` |
| `src/preprocessor.c` | 823 | `s[(*in)++]` |
| `src/preprocessor.c` | 844 | `(*in)++` |
| `src/preprocessor.c` | 882 | `(*in)++` |
| `src/preprocessor.c` | 907 | `(*in)++` |
| `src/preprocessor.c` | 932 | `(*in)++` |
| `src/preprocessor.c` | 967 | `(*in)++` |
| `src/preprocessor.c` | 992 | `(*in)++` |
| `src/preprocessor.c` | 1012 | `(*in)++` |
| `src/preprocessor.c` | 1032 | `(*in)++` |
| `src/preprocessor.c` | 1052 | `(*in)++` |
| `src/preprocessor.c` | 1072 | `(*in)++` |

### B-8 — `const` type qualifier in struct member declarations

The struct-member type parser does not accept the `const` qualifier; it
requires a bare type keyword:

```
include/lexer.h:7:5: error: expected integer type, got 'const'
```

The `Lexer` struct's first member (`const char *source`) triggers the
failure, leaving `Lexer` undefined.  Every reference to `Lexer` in
`lexer.c` then fails with `unknown type name 'Lexer'`.

| File | Line | Construct |
|---|---|---|
| `include/lexer.h` | 7 | `const char *source;` inside `typedef struct { … } Lexer` |

**Transitively blocked:** all of `src/lexer.c`.

---

## Successful compilation

### `version.c`

Compiles cleanly (`EXIT:0`, emits `version.asm`).  This module contains no
control-flow, no includes beyond simple macros, and uses no type qualifiers,
compound assignment, or subscript expressions — none of the current gaps apply.

---

## Feature gap summary

| Gap | C99 section | Affected modules |
|---|---|---|
| For-init declarations | §6.8.5.3 | `ast.c`, `ast_pretty_printer.c`, `type.c`, `preprocessor.c` |
| Enum/identifier `case` labels | §6.8.4.2 | `ast_pretty_printer.c`, `type.c` |
| Subscript of member-access expr | §6.5.2.1 | `ast.c`, `preprocessor.c` |
| Compound assignment operators | §6.5.16.2 | `preprocessor.c` |
| Hex escape sequences (`\x`) | §6.4.4.4 | `lexer.c`, `preprocessor.c` |
| Adjacent string literal concat | §6.4.5 | `preprocessor.c` |
| Postfix `++`/`--` on non-ident lvalue | §6.5.2.4 | `preprocessor.c` |
| `const` qualifier in struct members | §6.7.3 | `lexer.c` (via `lexer.h`) |
