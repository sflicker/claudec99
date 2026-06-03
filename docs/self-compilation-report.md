# Self-Compilation Diagnostic Report

**Date:** 2026-06-03
**Compiler:** `build/ccompiler`
**Flags:** `--max-errors=0 -Iinclude -Itest/include`

## Summary

| Module | Result | Root cause category |
|---|---|---|
| `ast.c` | PASS | — |
| `ast_pretty_printer.c` | PASS | — |
| `codegen.c` | PASS | — |
| `compiler.c` | PASS | — |
| `lexer.c` | FAIL | B — hexadecimal integer literals |
| `parser.c` | FAIL | B — address-of a struct member (`&s.m`) |
| `preprocessor.c` | PASS | — |
| `type.c` | PASS | — |
| `util.c` | PASS | — |
| `version.c` | PASS | — |

**8 of 10 modules compiled cleanly; 2 failed.** Both failures are
Category B (unimplemented language features). There were **no** Category A
failures — every system header the sources need already has a stub in
`test/include/`, and no resource limits (`MAX_*`) were hit.

---

## Category A — Missing stub system headers

None. All `#include <…>` directives in `src/` resolved against the stubs in
`test/include/` (`ctype.h`, `errno.h`, `limits.h`, `setjmp.h`, `stdarg.h`,
`stdbool.h`, `stddef.h`, `stdint.h`, `stdio.h`, `stdlib.h`, `string.h`,
`time.h`).

---

## Category B — Language features not yet implemented

### B1 — Hexadecimal integer literals (`0x…`)

**Affected module:** `lexer.c`

The lexer does not recognize the `0x`/`0X` hexadecimal integer-literal
prefix. When it encounters `0xFFu` it lexes the leading `0` as a decimal
integer and then treats the remaining `xFFu` as an *identifier*, producing
the root error:

```
src/lexer.c:271:52: error: expected ')', got identifier ('xFFu')
```

Minimal reproduction:

```c
int main(void){ unsigned int v = 0xFF; return (int)v; }
// error: expected ';', got identifier ('xFF')
```

Decimal literals and the `u`/`U` suffix on decimal literals both work
(`255u` compiles cleanly), so the gap is specifically the hex *prefix*, not
the suffix. Note that the recently added hex/octal *character escapes*
(`\xFF`, `\012`) are unrelated and unaffected — this gap is hex *integer
constants*.

This single root cause desynchronizes the parser and produces a cascade of
23 follow-on `error: expected type specifier` messages on later lines.

| Location | Construct |
|---|---|
| `lexer.c:271` | `(char)(val & 0xFFu)` |
| `lexer.c:282` | `(char)(val & 0xFFu)` |
| `lexer.c:360` | `(char)(val & 0xFFu)` |
| `lexer.c:371` | `(char)(val & 0xFFu)` |
| `lexer.c:487` | `(parsed <= 0xFFFFFFFFUL) ? …` |

C99 §6.4.4.1 (Integer constants — hexadecimal-constant).

### B2 — Address-of a struct/union member (`&s.member`)

**Affected module:** `parser.c`

The unary address-of operator rejects a `.` member-access operand, reporting
that the operand is not an lvalue:

```
src/parser.c:2921:72: error: address-of requires an lvalue
```

The reported position (line 2921, the closing brace of
`parse_parameter_list`) is **misattributed** — the `&` AST node carries a
stale source position. The construct that actually triggers the error is in
the *next* function, `parse_declaration_specifiers`:

```c
// src/parser.c:2989
r.base_type = parse_type_specifier(parser, &r.base_kind);
```

Here `r` is a local `DeclSpecResult` struct and `&r.base_kind` takes the
address of one of its members. Minimal reproduction:

```c
typedef struct { int k; } R;
void g(int *p);
void f(void){ R r; g(&r.k); }   // error: address-of requires an lvalue
```

Related forms that **do** work, which localizes the gap to member access
specifically:

- `&x` (plain variable) — compiles.
- `&a[1]` (array element / subscript) — compiles.

So the lvalue check for `&` recognizes identifiers and subscript
expressions, but not `.` (member-access) expressions. A member-access of an
lvalue is itself an lvalue in C99, so `&s.m` is well-formed.

`&r.base_kind` at `parser.c:2989` is the only occurrence of this pattern in
`src/`, which is why `parser.c` is the only module affected.

This single root cause desynchronizes the parser and produces a cascade of
6 follow-on `error: expected type specifier` messages.

| Location | Construct |
|---|---|
| `parser.c:2989` | `parse_type_specifier(parser, &r.base_kind)` |

C99 §6.5.3.2 (Address and indirection operators) with §6.3.2.1 (a member of
an lvalue structure is an lvalue).

---

## Successful compilation

The following 8 modules compiled cleanly to `.asm`:

| Module | Why it succeeded |
|---|---|
| `ast.c` | Uses only already-supported constructs; no hex literals, no `&member`. |
| `ast_pretty_printer.c` | Same — straightforward node-walking code. |
| `codegen.c` | Large, but stays within the supported subset. |
| `compiler.c` | Driver glue; no triggering constructs. |
| `preprocessor.c` | No hex literals or address-of-member usage. |
| `type.c` | Plain struct/enum manipulation. |
| `util.c` | Utility helpers only. |
| `version.c` | Trivial. |

The common thread: the two unimplemented features (hex integer literals and
`&s.member`) simply do not appear in these modules. The passing set confirms
the bulk of the language surface the compiler uses on itself is already
self-hosting.

---

## Feature gap summary

| Gap | C99 section | Affected modules |
|---|---|---|
| Hexadecimal integer literals (`0x…`) | §6.4.4.1 | `lexer.c` |
| Address-of a struct/union member (`&s.member`) | §6.5.3.2 / §6.3.2.1 | `parser.c` |

Closing these two gaps would bring every module in `src/` to a clean
self-compile.
