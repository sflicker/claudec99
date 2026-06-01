# Self-Compilation Diagnostic Report

**Date:** 2026-05-31
**Compiler:** `build/ccompiler`
**Flags:** `--max-errors=0 -Iinclude -Itest/include`

Each `src/*.c` file was compiled independently, in alphabetical order, with:

```
build/ccompiler --max-errors=0 -Iinclude -Itest/include src/<module>.c
```

No configurable limits (`MAX_FUNCTIONS`, etc.) were hit, so no `-D` overrides
were needed.

## Summary

| Module | Result | Root cause category |
|---|---|---|
| `ast.c` | FAIL | A — stub gap: `strncpy` not declared in `string.h` stub |
| `ast_pretty_printer.c` | PASS | — |
| `codegen.c` | FAIL | B — multidimensional array member in `codegen.h` |
| `compiler.c` | FAIL | B — same `codegen.h` multidimensional array member |
| `lexer.c` | FAIL | B — `\x` hex escape in a character literal |
| `parser.c` | FAIL | B — adjacent string-literal concatenation |
| `preprocessor.c` | FAIL | B — adjacent string-literal concatenation |
| `type.c` | PASS | — |
| `util.c` | FAIL¹ | B — **compiler crash (segfault)** on a file-scope array-typedef variable — **root cause fixed** (see B4) |
| `version.c` | PASS | — |

**3 passed, 7 failed.**

¹ The `util.c` segfault was traced to its root cause and fixed during this
investigation (see B4). After the fix, `util.c` no longer crashes the compiler;
it now stops with an ordinary Category-A diagnostic (`fseek` not declared in the
`stdio.h` stub). The result above reflects the pre-fix run that produced this
report.

---

## Category A — Missing / incomplete stub system headers

### `string.h` stub is missing several declarations

The bundled `test/include/string.h` declares only:

```
strcmp, strlen, memcpy, memset, memcmp, strchr
```

The project source uses additional standard `<string.h>` functions that have
**no declaration** in the stub, so the compiler reports them as undefined:

```
src/ast.c:14:19: error: call to undefined function 'strncpy'
```

Functions used in `src/` but absent from the stub:

| Function | Used in (examples) |
|---|---|
| `strncpy` | `ast.c:14`, `parser.c:1107` |
| `strncat` | `util.c:107` |
| `strncmp` | several modules |
| `strcpy`  | several modules |
| `strrchr` | `util.c:103` |

This is **not a compiler bug** — the stub header simply hasn't been extended
with these prototypes yet. Note this is a *missing declaration in an existing
stub*, not an `include file not found` error (the header file itself resolves).

`ast.c` is the only module whose *first / dominant* error is this stub gap; it
parses fully and then fails the semantic "undefined function" check. In
`parser.c` and `util.c` an earlier parse error or codegen crash (Category B)
fires first, so their `strncpy`/`strncat` usages never reach the semantic
stage.

**Fix:** add the missing prototypes to `test/include/string.h`.

---

## Category B — Language features not yet implemented

### B1 — Multidimensional (2-D) array declarators

`include/codegen.h:113` declares a 2-D array struct member:

```c
char user_labels[MAX_USER_LABELS][MAX_NAME_LEN];
```

After parsing the first `[…]` dimension the parser expects the member to end:

```
include/codegen.h:113:25: error: expected ';', got '[' ('[')
```

Both `MAX_USER_LABELS` (64) and `MAX_NAME_LEN` (256) are defined in
`include/constants.h`, so this is genuinely a missing language feature, not an
undefined macro. Minimal reproducer:

```c
struct S { char m[4][8]; };   // error: expected ';', got '['
```

Because the failure is in the shared header `codegen.h`, every translation
unit that includes it is blocked. The single root-cause error cascades into a
long run of `expected type specifier` / `unknown type name 'CodeGen'` errors
(the parser loses the `CodeGen` typedef once the struct body fails).

| File | First error |
|---|---|
| `codegen.h:113` | `expected ';', got '['` (root cause) |
| `codegen.c` | inherits via `#include "codegen.h"` (cascade from line 154 on) |
| `compiler.c` | inherits via `#include "codegen.h"` (cascade from line 154 on) |

C99 §6.7.5.2 (array declarators), §6.5.2.1 (subscripting).

### B2 — Adjacent string-literal concatenation

C99 concatenates adjacent string literals into one. The compiler does not, so
a split format string inside a call is read as two arguments and the call's
`)` is never matched:

```c
PARSER_ERROR(parser,
        "error: too many parameters in function pointer"
        " type (max %d)\n", FUNC_TYPE_MAX_PARAMS);
```

```
src/parser.c:1108:37: error: expected ')', got string literal (' type (max %d)\n')
src/preprocessor.c:603:33: error: expected ')', got string literal (' expected %s%d, got %d\n')
```

Minimal reproducer:

```c
fprintf(stderr, "abc" "def\n");   // error: expected ')', got string literal
```

This single parse failure produces the large cascade of `expected type
specifier` errors that follows in both files.

| File | First error line |
|---|---|
| `parser.c` | `1108` (cascades through 1170+, again at 2830+) |
| `preprocessor.c` | `603` (cascades through 689+) |

C99 §6.4.5 (string literals — translation phase 6).

### B3 — Hex escape sequences in character literals

`lexer.c` uses `'\x01'` (the in-band include-boundary marker). The compiler's
own character-literal lexer has a `switch` on the escape char that handles
`\a \b \f \n \r \t \v \\ \' \" \? \0` but has no `\x` (hex) case, so it falls
through to its `default` and aborts:

```
error: invalid escape sequence in character literal
```

(No file:line is attached because this is emitted by the lexer's own
`fprintf(stderr, …); exit(1)` path rather than a positioned diagnostic.)

Affected: `src/lexer.c` (e.g. lines 68, 101 — `'\x01'`).

C99 §6.4.4.4 (character constants — hexadecimal-escape-sequence).

### B4 — Compiler crash (segfault) on a file-scope array-typedef variable

`util.c` does **not** produce a diagnostic — `build/ccompiler` terminates with
SIGSEGV (exit 139) and no output. This is a codegen crash, not a clean
rejection, and reproduces deterministically.

Bisection pinned it to the error-recovery globals plus `do_compile_error`,
and minimisation reduced it to **a reference to a file-scope (global) variable
whose type is an array typedef**. In `util.c` that is:

```c
jmp_buf g_error_jmp;          // util.h: typedef long jmp_buf[32];
...
longjmp(g_error_jmp, 1);      // any use of g_error_jmp triggers the crash
```

Minimal reproducers (3 lines each):

```c
typedef long buf_t[32];
buf_t jb;
void f(void) { jb[0] = 1; }   // SIGSEGV
```

```c
typedef long buf_t[32];
buf_t jb;
long *f(void) { return jb; }  // SIGSEGV (array-to-pointer decay of the global)
```

Boundary analysis — what does **not** crash:

| Variant | Result |
|---|---|
| `typedef long buf_t[32]; buf_t jb;` declared but **never referenced** | OK |
| Same typedef as a **local** variable, then used (`buf_t jb; g(jb);`) | OK |
| Plain (non-typedef) **global** array `long arr[32];` used as an argument | OK |
| **Global** array-typedef variable **referenced** in any function body | **SIGSEGV** |

So the crash needs all three of: (1) file-scope storage, (2) the array type
introduced through a *typedef*, and (3) at least one *use* of the variable in a
function body. Codegen appears to dereference a null symbol/type record when
resolving a global whose declared type is an array typedef (plain global arrays
and local array-typedef vars take a different, working path).

This is the most serious finding: it is a **compiler crash**, not an
unimplemented-feature diagnostic, and `setjmp`/`longjmp`-based error recovery
in `util.c` cannot self-compile until it is fixed.

Affected: `src/util.c` (`g_error_jmp`, used in `src/util.c:30` via `longjmp`).

#### Root cause (traced) and fix

A `gdb` backtrace put the fault at `src/codegen.c:719`, in
`emit_array_index_addr`:

```c
if (gv->kind == TYPE_ARRAY) {
    element = gv->full_type->base;   /* gv->full_type == NULL  → SIGSEGV */
```

The global's `kind` was `TYPE_ARRAY` but its `full_type` was `NULL`.
`codegen_add_global` copies that field straight from the AST
(`gv->full_type = decl->full_type;`), so the real defect is upstream in the
**parser**.

`parse_external_declaration` (file-scope) had branches for `TYPE_POINTER`,
`TYPE_STRUCT`, and `TYPE_UNION` but **no `TYPE_ARRAY` branch** — whereas the
block-scope (local) declaration parser does (`src/parser.c:2548`). That
asymmetry is exactly why a `buf_t jb;` *local* compiled while the *global*
crashed.

For a variable declared through an array typedef the declarator carries no
`[...]`, so `d.is_array` is false and the `d.is_array` block (which builds and
attaches the array `full_type`) is skipped. The array-ness comes only from
`base_kind == TYPE_ARRAY`, which fell through to the scalar `else`:

```c
} else {
    decl->decl_type = base_kind;                /* = TYPE_ARRAY            */
    decl->is_unsigned = !base_type->is_signed;  /* arrays aren't signed →  */
}                                               /* spurious "unsigned";    */
                                                /* full_type NEVER set     */
```

This is visible in the pre-fix AST as `VariableDeclaration: unsigned array jb`
(no `[32]`, plus a bogus `unsigned` flag) versus the local form
`VariableDeclaration: long jb[32]`.

**Fix** — add the missing array branch in `parse_external_declaration`,
mirroring the block-scope path (`full_type` is already the array `Type*`
returned by `parse_type_specifier`):

```c
} else if (base_kind == TYPE_ARRAY) {
    decl->decl_type = TYPE_ARRAY;
    decl->full_type = full_type;
}
```

**Verification:**

- AST now resolves correctly: `VariableDeclaration: long jb[32]`.
- All three minimal reproducers above exit 0.
- `util.c` no longer segfaults; it now stops with an ordinary diagnostic
  (`error: call to undefined function 'fseek'` — a Category-A stub gap).
- Full test suite: **1263 passed, 0 failed** (no regression; plain non-typedef
  array globals were never affected, since their `full_type` is built in the
  `d.is_array` block).

---

## Successful compilation

| Module | Why it compiled |
|---|---|
| `ast_pretty_printer.c` | Uses only already-supported syntax; its `string.h` needs (`strchr`, `strlen`) are present in the stub, and it has no 2-D arrays, literal concatenation, hex char escapes, or array-typedef globals. |
| `type.c` | Same — stays within the supported C subset and the available stub declarations. |
| `version.c` | Tiny generated/static module with no problematic constructs. |

These three confirm the toolchain end-to-end (lex → parse → codegen → `.asm`
emission) works for the subset of C99 they use.

---

## Feature gap summary

| Gap | C99 section | Affected modules |
|---|---|---|
| Incomplete `string.h` stub (`strncpy`, `strncat`, `strncmp`, `strcpy`, `strrchr`) | library, §7.24 | `ast.c` (dominant); used latently in `parser.c`, `util.c` |
| Multidimensional array declarators (`m[A][B]`) | §6.7.5.2, §6.5.2.1 | `codegen.h` → `codegen.c`, `compiler.c` |
| Adjacent string-literal concatenation (`"a" "b"`) | §6.4.5 | `parser.c`, `preprocessor.c` |
| Hex escape in character literal (`'\x01'`) | §6.4.4.4 | `lexer.c` |
| **Codegen crash** (now **fixed**): reference to a file-scope array-typedef variable — missing `TYPE_ARRAY` branch in `parse_external_declaration` | §6.7.7 / §6.3.2.1 | `util.c` |
