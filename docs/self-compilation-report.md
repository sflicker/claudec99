# Self-Compilation Diagnostic Report

**Date:** 2026-05-25  
**Compiler:** `build/ccompiler`  
**Flags:** `--max-errors=0 -Iinclude -Itest/include`

Each source module in `src/` was compiled one at a time using the built
compiler.  This report documents every failure, its root cause, and its
category.

---

## Summary

| Module | Result | Root cause category |
|---|---|---|
| `ast.c` | FAIL | Feature gap (2 kinds) |
| `ast_pretty_printer.c` | FAIL | Feature gap (2 kinds) |
| `codegen.c` | FAIL | Missing stub header |
| `compiler.c` | FAIL | Missing stub header |
| `lexer.c` | FAIL | Missing stub headers (2) |
| `parser.c` | FAIL | Missing stub header |
| `preprocessor.c` | FAIL | Missing stub headers (2) |
| `type.c` | FAIL | Feature gap (2 kinds) |
| `util.c` | FAIL | Missing stub header |
| `version.c` | **PASS** | — |

---

## Category A — Missing stub system headers

The test stub directory (`test/include/`) provides: `limits.h`, `stdbool.h`,
`stddef.h`, `stdint.h`, `stdio.h`, `stdlib.h`, `string.h`.

The following headers are required by the compiler's own source but have no
stub and are not found anywhere on the compiler's include search path.

### `setjmp.h`

**Needed by:** `util.h` (line 4) — which is included by `util.c`, `codegen.c`,
`compiler.c`, and `parser.c`.

`util.h` declares `extern jmp_buf g_error_jmp` and `util.c` defines it.
`jmp_buf` is a typedef from `<setjmp.h>`.  Because the preprocessor fails on
the very first unresolved `#include`, all four files produce only a single
error and no further diagnostics.

```
error: include file not found: <setjmp.h>
```

**Classification:** Missing stub — not a language/compiler bug.

---

### `ctype.h`

**Needed by:** `lexer.c` (line 1), `preprocessor.c` (line 1).

Both files use `isalpha()`, `isdigit()`, `isalnum()`, `isspace()`, and
`isupper()`/`islower()`.  These are macros or inline functions defined in
`<ctype.h>`.

```
error: include file not found: <ctype.h>
```

**Classification:** Missing stub — not a language/compiler bug.

---

### `errno.h`

**Needed by:** `lexer.c` (line 2).

`lexer.c` uses `errno` and the `ERANGE` constant (for overflow detection in
`strtoul`) after including `<errno.h>`.  The preprocessor stops at `<ctype.h>`
first (line 1), so `errno.h` is never reached; but it would fail too.

**Classification:** Missing stub — not a language/compiler bug.

---

### `time.h`

**Needed by:** `preprocessor.c` (line 5).

`preprocessor.c` includes `<time.h>` to support the `__DATE__`/`__TIME__`
predefined macros (or similar time-stamped output).  The preprocessor stops at
`<ctype.h>` first, so `time.h` is never reached; but it would fail too.

**Classification:** Missing stub — not a language/compiler bug.

---

## Category B — Language features not yet implemented

These errors occur in files whose system includes are all available
(`stdio.h`, `stdlib.h`, `string.h`, and project headers).

### B-1  C99 for-loop variable declarations

**Pattern:** `for (int i = 0; ...)`  
**Error:** `error: expected expression, got 'int'`

C99 allows a declaration in the first clause of a `for` statement.  The
compiler's parser treats that clause as an expression and rejects `int` where
it expects an expression.

**Affected locations:**

| File | Line | Code |
|---|---|---|
| `ast.c` | 27 | `for (int i = 0; i < node->child_count; i++)` |
| `ast_pretty_printer.c` | 29 | `for (int i = 0; i < depth * 2; i++)` |
| `ast_pretty_printer.c` | 149 | `for (int i = 0; i < byte_length; i++)` |
| `ast_pretty_printer.c` | 197 | `for (int i = 0; i < node->child_count; i++)` |
| `ast_pretty_printer.c` | 280 | `for (int i = 0; i < node->child_count; i++)` |
| `type.c` | 95 | `for (int i = 0; i < param_count && i < FUNC_TYPE_MAX_PARAMS; i++)` |

Each `for`-init declaration failure causes the parser to lose sync, producing
a cascade of spurious "expected type specifier" errors for all subsequent
statements in the same function.

**Classification:** Feature not yet implemented — C99 §6.8.5.3 for-statement
with declaration in the init clause.

---

### B-2  Enum values (and other constant expressions) as `case` labels

**Pattern:** `case ENUM_CONSTANT:`  
**Error:** `error: expected integer literal, got identifier ('...')`

The parser requires an integer literal in a `case` label.  C99 requires only
an *integer constant expression*, which includes enum values, macro-expanded
constants, and `sizeof` expressions.  The compiler does not evaluate constant
expressions in this position, so any `case` label that is not a bare decimal
literal fails.

**Affected locations:**

| File | Line | Identifier |
|---|---|---|
| `ast_pretty_printer.c` | 69 | `AST_TRANSLATION_UNIT` |
| `ast_pretty_printer.c` | 72 | `AST_FUNCTION_DECL` |
| `ast_pretty_printer.c` | 83 | `AST_PARAM` |
| `ast_pretty_printer.c` | 94 | `AST_BLOCK` |
| … (many more) | | all `AST_*` and `TYPE_*` enum values |
| `type.c` | 147 | `TYPE_VOID` |
| `type.c` | 148 | `TYPE_BOOL` |
| … (many more) | | all `TYPE_*` enum values |

`ast_pretty_printer.c` produces over 100 errors from this single root cause.

**Classification:** Feature not yet implemented — C99 §6.8.4.2 requires
constant expressions (not just literals) as `case` labels.

---

### B-3  Subscripting a member-access expression

**Pattern:** `expr->field[index]` or `expr.field[index]`  
**Error:** `error: subscript base must be an identifier`

`ast.c:21` writes `parent->children[parent->child_count++] = child`.  The
compiler's parser accepts subscript (`[]`) only when the left operand is a
plain identifier.  It does not support subscripting the result of `->` or `.`
member access, even though C99 treats `a[i]` as `*(a+i)` and the left operand
may be any pointer expression.

**Affected locations:**

| File | Line | Code |
|---|---|---|
| `ast.c` | 21 | `parent->children[parent->child_count++]` |

**Classification:** Feature not yet implemented — C99 §6.5.2.1 postfix
subscript applies to any pointer expression as the first operand.

---

## Successful compilation

### `version.c`

Compiles and links without error:

```
compiled: src/version.c -> version.asm
```

`version.c` uses only `stdio.h` and `stdlib.h` (both stubbed), has no
for-loop declarations, and contains no switch/case on enum values.

---

## Feature gap summary

| Gap | C99 section | Affected modules |
|---|---|---|
| Missing `setjmp.h` stub | — | `util.c`, `codegen.c`, `compiler.c`, `parser.c` |
| Missing `ctype.h` stub | — | `lexer.c`, `preprocessor.c` |
| Missing `errno.h` stub | — | `lexer.c` |
| Missing `time.h` stub | — | `preprocessor.c` |
| `for`-init declarations | §6.8.5.3 | `ast.c`, `ast_pretty_printer.c`, `type.c` |
| Enum values in `case` labels | §6.8.4.2 | `ast_pretty_printer.c`, `type.c` |
| Subscript of member-access expr | §6.5.2.1 | `ast.c` |
