# Self-Compilation Diagnostic Report

**Date:** 2026-06-02
**Compiler:** `build/ccompiler`
**Flags:** `--max-errors=0 -Iinclude -Itest/include`

## Summary

| Module | Result | Root cause category |
|---|---|---|
| `ast.c` | PASS | — |
| `ast_pretty_printer.c` | PASS | — |
| `codegen.c` | FAIL | B — adjacent string literal concatenation |
| `compiler.c` | PASS | — |
| `lexer.c` | FAIL | B — escape sequences in character literals |
| `parser.c` | FAIL | B — adjacent string literal concatenation |
| `preprocessor.c` | FAIL | B — adjacent string literal concatenation |
| `type.c` | PASS | — |
| `util.c` | FAIL | A — incomplete `stdio.h` stub |
| `version.c` | PASS | — |

**5 modules passed, 5 failed.**

---

## Category A — Missing / incomplete stub system headers

### `stdio.h` — missing file-positioning and binary-I/O declarations

`src/util.c` reads a whole file into memory using the standard
`fseek`/`ftell`/`fread` idiom:

```c
fseek(f, 0, SEEK_END);
long len = ftell(f);
fseek(f, 0, SEEK_SET);
...
fread(buf, 1, len, f);
```

First error:

```
src/util.c:78:13: error: call to undefined function 'fseek'
```

The existing `test/include/stdio.h` stub declares `fopen`, `fclose`, `fgetc`,
`fgets`, `fprintf`, and `vfprintf`, but **not** `fseek`, `ftell`, or `fread`,
and it does not define the `SEEK_SET` / `SEEK_END` macros. The header exists —
it is simply incomplete. The compiler aborts code generation at the first
undefined function (`fseek`), so the later `ftell`/`fread` uses are not yet
reported, but they have the same cause.

Symbols `util.c` needs that are absent from the stub:

| Symbol | Line in `util.c` | Kind |
|---|---|---|
| `fseek` | 78, 80 | function |
| `SEEK_END` | 78 | macro |
| `ftell` | 79 | function |
| `SEEK_SET` | 80 | macro |
| `fread` | 88 | function |

**Fix:** add these declarations/macros to `test/include/stdio.h`. This is not a
compiler bug.

---

## Category B — Language features not yet implemented

### B1 — Adjacent string literal concatenation (C99 §6.4.5)

The compiler does not concatenate adjacent string-literal tokens. When it sees a
second string literal where it expects a `,` or `)` (or `;`), it errors.
Minimal reproduction:

```c
const char *s = "ab" "cd";
/* error: expected ';', got string literal ('cd') */
```

This single gap accounts for the first (root) error in three modules; every
subsequent diagnostic in those files (`expected type specifier`,
`non-constant initializer for global ...`, `address-of requires an lvalue`,
etc.) is **cascade** from the parser losing sync at the split string.

Affected root-cause locations:

| File:line | Split-string call |
|---|---|
| `src/codegen.c:3293` | `compile_error("error: va_arg: type %s is not supported;" " use int or a larger type\n", ...)` |
| `src/parser.c:1156` | `PARSER_ERROR(parser, "error: too many parameters in function pointer" " type (max %d)\n", ...)` |
| `src/preprocessor.c:603` | `fprintf(stderr, "error: argument count mismatch for macro '%.*s':" " expected %s%d, got %d\n", ...)` |

(The reported column on the cascade differs slightly from the source line
because the lexer counts the line of the *second* literal; the `codegen.c` first
error is emitted as `3256:27` but the offending construct is the multi-line
literal ending at line 3293.)

### B2 — Escape sequences in character literals (C99 §6.4.4.4)

`src/lexer.c` uses hex escapes in character constants, e.g.
`if (c == '\x01')` (line 101; also referenced in the comment on line 68). The
lexer rejects them:

```
error: invalid escape sequence in character literal
```

Minimal reproductions:

```c
char c = '\x01';   /* error: invalid escape sequence in character literal */
char c = '\001';   /* error: multi character constant                     */
```

So neither **hex** (`\xhh`) nor **octal** (`\ooo`) numeric escape sequences are
recognized inside character literals. (Simple escapes such as `'\n'`, `'\0'`,
`'\t'` are accepted — these appear throughout `lexer.c` without error.) The lone
unsupported hex escape on line 101 is enough to fail the whole module; the
compiler stops at the first bad character literal.

---

## Successful compilation

| Module | Why it compiled |
|---|---|
| `ast.c` | Uses only already-stubbed headers and no unimplemented syntax. |
| `ast_pretty_printer.c` | Same — no split string literals or numeric char escapes. |
| `compiler.c` | Driver glue; stays within the supported language subset. |
| `type.c` | Plain type tables/helpers; supported constructs only. |
| `version.c` | Trivial; version strings only. |

These modules happen to avoid both feature gaps (no adjacent string-literal
concatenation, no hex/octal character escapes) and require no stub symbols that
are missing.

---

## Feature gap summary

| Gap | C99 section | Affected modules |
|---|---|---|
| Adjacent string literal concatenation | §6.4.5 | `codegen.c`, `parser.c`, `preprocessor.c` |
| Hex / octal escape sequences in character literals | §6.4.4.4 | `lexer.c` |
| Incomplete `stdio.h` stub (`fseek`/`ftell`/`fread`, `SEEK_SET`/`SEEK_END`) | — (stub, not language) | `util.c` |

No `constants.h` limits (e.g. `MAX_FUNCTIONS`) were hit; no `-D` overrides were
needed for any module.
