# Self-Compilation Diagnostic Report

**Date:** 2026-05-31
**Compiler:** `build/ccompiler` (version 00.01.00840000, stage-84)
**Flags:** `--max-errors=0 -Iinclude -Itest/include`

## Summary

| Module | Result | Root cause category |
|---|---|---|
| `ast.c` | FAIL | A — `exit` not declared in `stdlib.h` stub |
| `ast_pretty_printer.c` | FAIL | B — array-typed struct member doesn't decay to pointer |
| `codegen.c` | FAIL | B — 2D array declaration in `codegen.h` |
| `compiler.c` | FAIL | B — 2D array declaration in `codegen.h` (transitive) |
| `lexer.c` | FAIL | B — `\xNN` hex escape sequences not supported |
| `parser.c` | FAIL | B — adjacent string literal concatenation not supported |
| `preprocessor.c` | FAIL | B — adjacent string literal concatenation not supported |
| `type.c` | **PASS** | — |
| `util.c` | FAIL | A — `exit` not declared in `stdlib.h` stub |
| `version.c` | **PASS** | — |

**2 passed, 8 failed**

---

## Category A — Missing stub system headers

### A.1 — `exit()` not declared in `test/include/stdlib.h`

The stub `test/include/stdlib.h` exists but only declares `malloc`, `realloc`,
`calloc`, and `free`. The `exit()` function (C99 §7.20.4.3) is absent.
The compiler therefore reports `error: call to undefined function 'exit'`
for any module that calls it.

**Directly blocked modules:**

| Module | Error |
|---|---|
| `ast.c` | `error: call to undefined function 'exit'` |
| `util.c` | `error: call to undefined function 'exit'` |

**Secondarily blocked (blocked by a B-category error first, but would also
fail here once that is fixed):**

| Module | Notes |
|---|---|
| `preprocessor.c` | Calls `exit(1)` in several error paths |

**Fix:** Add `void exit(int);` (and optionally `void abort(void);`) to
`test/include/stdlib.h`.

---

## Category B — Language features not yet implemented

### B.1 — 2D array declarations (`T name[M][N]`)

`include/codegen.h:113` declares:

```c
char user_labels[MAX_USER_LABELS][MAX_NAME_LEN];
```

The compiler does not support multi-dimensional array declarations (arrays
of arrays). Parsing fails at the second `[`, producing:

```
include/codegen.h:113:25: error: expected ';', got '[' ('[')
```

Because the `CodeGen` struct definition is incomplete, every subsequent
declaration or use of `CodeGen` cascades into `error: unknown type name 'CodeGen'`.

**Directly blocked modules:**

| Module | Root error location |
|---|---|
| `codegen.c` | `include/codegen.h:113` — includes codegen.h directly |
| `compiler.c` | `include/codegen.h:113` — includes codegen.h transitively |

The cascade produces 43 `unknown type name 'CodeGen'` errors in `codegen.c`
and 9 errors in `compiler.c` after the single root failure.

---

### B.2 — Adjacent string literal concatenation (`"str1" "str2"`)

C99 §6.4.5 requires that adjacent string literal tokens be concatenated into
a single token before parsing. The compiler does not implement this; each
literal is treated as a separate expression.

The pattern appears in `PARSER_ERROR`/`compile_error` macro call sites where
long error strings are split across lines for readability, e.g.:

```c
// parser.c lines 1136-1138
PARSER_ERROR(parser,
        "error: too many parameters in function pointer"
        " type (max %d)\n", FUNC_TYPE_MAX_PARAMS);
```

After macro expansion, the compiler encounters two string literal tokens where
it expects one argument, producing:

```
src/parser.c:1094:37: error: expected ')', got string literal (' type (max %d)\n')
```

Similarly in `preprocessor.c`:
```c
// preprocessor.c lines 601-603
fprintf(stderr,
        "error: argument count mismatch for macro '%.*s':"
        " expected %s%d, got %d\n", ...);
```

```
src/preprocessor.c:603:33: error: expected ')', got string literal (...)
```

**Note:** `codegen.c` also uses adjacent string literals (line 3134), but is
blocked earlier by the 2D-array failure (B.1).

**Directly blocked modules (first error):**

| Module | First error location |
|---|---|
| `parser.c` | `parser.c:1094` (macro-expanded from lines 1136–1138) |
| `preprocessor.c` | `preprocessor.c:603` |

---

### B.3 — Hex escape sequences (`\xNN`) not supported

C99 §6.4.4.4 requires hexadecimal escape sequences of the form `\xNN` in
character and string literals. The compiler only handles the named simple
escapes (`\n`, `\t`, `\r`, `\\`, `\"`, `\'`, `\a`, `\b`, `\f`, `\v`, `\?`, `\0`)
and rejects `\x` with:

```
error: invalid escape sequence in character literal
```

`lexer.c` uses `'\x01'` as a sentinel byte (the location marker injected by
the preprocessor at `#include` boundaries). E.g.:

```c
// lexer.c:101
if (c == '\x01') { ...
```

`preprocessor.c` also uses `"\x01"` (in a string literal) as the prefix of
that same marker. `preprocessor.c` is blocked by B.2 first, but would also
fail here once B.2 is fixed.

**Directly blocked modules (first error):**

| Module | Error | Location |
|---|---|---|
| `lexer.c` | `invalid escape sequence in character literal` | `lexer.c:101` |

**Secondarily blocked (blocked by B.2 first):**

| Module | Notes |
|---|---|
| `preprocessor.c` | Uses `"\x01"` string literal at lines 1381, 1392 |

---

### B.4 — Array-typed struct member does not decay to pointer

C99 §6.3.2.1 requires that an array lvalue decays to a pointer to its first
element in most expression contexts, including function-call arguments. The
compiler does not apply this decay when the array is accessed as a struct
member via `.` or `->`.

When `node->value` (where `value` is `char value[MAX_NAME_LEN]` in `ASTNode`)
is passed to a function expecting `const char *`, the compiler reports:

```
error: function 'operator_name' parameter 'op' expected pointer argument, got integer
```

The compiler resolves the member type as `char` (the element type) instead of
`char *` (the decayed pointer), classifying it as an integer argument.

The same failure reproduces with dot access (`n.value`):

```c
// ast_pretty_printer.c:168
printf("Binary: %s\n", operator_name(node->value));
```

**Directly blocked modules:**

| Module | Error location |
|---|---|
| `ast_pretty_printer.c` | Call sites at lines 168, 171, 231, 234 |

**Other modules blocked once prior issues are fixed:** `parser.c` and
`preprocessor.c` (and others) use `name.value` / `token->value` patterns
extensively; the same array-member-decay gap would surface after B.1–B.3 are
resolved.

---

## Successful compilation

### `type.c` — PASS

`type.c` uses only basic scalar variables, pointer operations, struct member
access, and simple control flow. It has no calls to `exit`, no multi-dimensional
arrays, no hex escape sequences, and no adjacent string literals. All
dependencies (`type.h`, `stddef.h`) are available via the project include paths.

### `version.c` — PASS

`version.c` uses only `snprintf` (declared in `stdio.h` stub) and a simple
`if`/`else` branch. It has no dependency on any unsupported feature.

---

## Feature gap summary

| Gap | C99 section | Affected modules (direct) |
|---|---|---|
| `exit()` missing from `stdlib.h` stub | §7.20.4.3 | `ast.c`, `util.c` |
| 2D array declarations (`T a[M][N]`) | §6.7.5.2 | `codegen.c`, `compiler.c` |
| Adjacent string literal concatenation | §6.4.5 | `parser.c`, `preprocessor.c` |
| Hex escape sequences (`\xNN`) | §6.4.4.4 | `lexer.c` |
| Array member access decay to pointer | §6.3.2.1 | `ast_pretty_printer.c` |
