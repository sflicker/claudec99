# ClaudeC99 Stage 105 — C99 Preprocessor Completion

## Goal

Complete the C99 preprocessing feature set by adding five items that are
currently missing or rejected:

```c
/* 1. #pragma — unknown pragmas silently ignored; #pragma once supported */
#pragma once
#pragma GCC optimize("O2")          /* silently ignored */

/* 2. _Pragma operator (C99 §6.10.9) */
_Pragma("once")                     /* equivalent to #pragma once */

/* 3. #line directive (C99 §6.10.4) */
#line 42 "generated.c"              /* override __LINE__ and __FILE__ */

/* 4. Null directive (C99 §6.10.7) */
#                                   /* bare # on a line — silently ignored */

/* 5. __func__ predefined identifier (C99 §6.4.2.2) */
void greet(void) {
    printf("in %s\n", __func__);    /* prints "in greet" */
}
```

Currently:
- `#pragma` hits the `"error: unsupported preprocessor directive"` fallthrough.
- `_Pragma(...)` is treated as an unknown identifier (passed through to the parser).
- `#line` hits the same unsupported-directive error.
- A bare `#` on its own line hits the same unsupported-directive error.
- `__func__` resolves as an undeclared identifier, producing a parse error.

This stage covers three modules: the **preprocessor** (`src/preprocessor.c`),
the **parser** (`src/parser.c` and `include/parser.h`), and **version**
(`src/version.c`).  No new AST node types are needed.

---

## Background — current state

### Preprocessor directive dispatch (`src/preprocessor.c`)

`preprocess_internal` handles directives in a chain of `strncmp` checks.
After `#ifdef`, `#ifndef`, `#if`, `#elif`, `#else`, `#endif`, there is an
inactive-region guard:

```c
/* In inactive regions, skip all remaining directives without error. */
if (!emitting) {
    while (s[in] && s[in] != '\n') in++;
    continue;
}
```

Then come `#include`, `#define`, `#undef`, `#error`.  After the last handler:

```c
/* All other directives are unsupported. */
fprintf(stderr, "error: unsupported preprocessor directive\n");
free(out.data);
free(spliced);
exit(1);
```

Null directives (`#` alone) also fall through to this error because no
keyword match fires.

### `_Pragma` / `__func__` in identifier expansion

Identifiers are handled after all directives, where `__FILE__` and `__LINE__`
are special-cased.  `_Pragma` and `__func__` reach neither special case and
are passed through unchanged.

### `__LINE__` / `__FILE__` tracking

`current_line` is a local `int` in `preprocess_internal` (starts at 1,
incremented on every `\n`).  `__FILE__` uses `source_path`, a `const char *`
parameter.  A `#line` directive must be able to override both values
independently of the real file structure.

### `MacroTable` structure

```c
typedef struct {
    MacroDef *defs;
    size_t    count;
    size_t    cap;
} MacroTable;
```

`MacroTable` is the shared mutable state threaded through all recursive
preprocessing calls.  It is the natural home for `#pragma once` tracking.

---

## C99 references

| Feature | C99 standard section |
|---------|----------------------|
| `#pragma` | §6.10.6 |
| `_Pragma` operator | §6.10.9 |
| `#line` | §6.10.4 |
| Null directive | §6.10.7 |
| `__func__` | §6.4.2.2 |

---

## Grammar

No new tokens or grammar productions.  `__func__` reuses `AST_STRING_LITERAL`
(synthesized in `parse_primary`).

---

## Task 1 — Add `#pragma once` tracking to `MacroTable` (`src/preprocessor.c`)

### 1a. Extend `MacroTable`

Add three fields to the `MacroTable` struct:

```c
typedef struct {
    MacroDef *defs;
    size_t    count;
    size_t    cap;
    char    **once_paths;   /* canonical paths where #pragma once was seen */
    size_t    once_count;
    size_t    once_cap;
} MacroTable;
```

### 1b. Update `macro_table_init` and `macro_table_free`

In `macro_table_init`, zero-initialize the new fields:

```c
static void macro_table_init(MacroTable *t) {
    t->defs        = NULL;
    t->count       = 0;
    t->cap         = 0;
    t->once_paths  = NULL;
    t->once_count  = 0;
    t->once_cap    = 0;
}
```

In `macro_table_free`, free the once-paths array and each stored path string:

```c
/* free once_paths entries */
for (size_t i = 0; i < t->once_count; i++)
    free(t->once_paths[i]);
free(t->once_paths);
```

### 1c. Add `macro_table_add_once` and `macro_table_is_once` helpers

```c
static void macro_table_add_once(MacroTable *t, const char *path) {
    /* grow if needed */
    if (t->once_count >= t->once_cap) {
        size_t new_cap = t->once_cap == 0 ? 8 : t->once_cap * 2;
        char **np = realloc(t->once_paths, new_cap * sizeof(char *));
        if (!np) { fprintf(stderr, "error: out of memory\n"); exit(1); }
        t->once_paths = np;
        t->once_cap   = new_cap;
    }
    t->once_paths[t->once_count++] = util_strdup(path);
}

static int macro_table_is_once(const MacroTable *t, const char *path) {
    for (size_t i = 0; i < t->once_count; i++)
        if (strcmp(t->once_paths[i], path) == 0)
            return 1;
    return 0;
}
```

`util_strdup` is declared in `include/util.h` and already used elsewhere in
`preprocessor.c`.

---

## Task 2 — Check `#pragma once` in `preprocess_file` (`src/preprocessor.c`)

At the top of `preprocess_file`, before reading the file, add:

```c
static char *preprocess_file(const char *path, int depth, MacroTable *macros,
                              const char **include_dirs, int n_include_dirs) {
    /* #pragma once: return empty string for a file already processed */
    if (macro_table_is_once(macros, path)) {
        char *empty = malloc(1);
        if (!empty) { fprintf(stderr, "error: out of memory\n"); exit(1); }
        empty[0] = '\0';
        return empty;
    }
    /* ... existing code ... */
```

---

## Task 3 — Add `#pragma` directive handler (`src/preprocessor.c`)

In `preprocess_internal`, add the `#pragma` handler immediately **before** the
`"error: unsupported preprocessor directive"` fallthrough.

```c
/* #pragma ... */
if (strncmp(s + in, "pragma", 6) == 0 &&
    !isalnum((unsigned char)s[in + 6]) && s[in + 6] != '_') {
    in += 6;
    while (s[in] == ' ' || s[in] == '\t') in++;
    /* #pragma once: record this file so subsequent includes are skipped. */
    if (strncmp(s + in, "once", 4) == 0 &&
        (s[in + 4] == '\n' || s[in + 4] == '\0' ||
         s[in + 4] == ' '  || s[in + 4] == '\t')) {
        if (source_path)
            macro_table_add_once(macros, source_path);
    }
    /* All #pragma directives (including once): consume to end of line. */
    while (s[in] && s[in] != '\n') in++;
    continue;
}
```

**Notes:**
- The `!emitting` guard earlier in the chain already handles inactive regions
  (directives inside a false `#if` branch are skipped before this code is
  reached), so no special inactive-region handling is needed here.
- Recognized C99 STDC pragmas (`STDC FP_CONTRACT`, `STDC FENV_ACCESS`,
  `STDC CX_LIMITED_RANGE`) are silently ignored along with all other unknown
  pragmas — ClaudeC99 does not implement floating-point semantics.

---

## Task 4 — Add `#line` directive handler (`src/preprocessor.c`)

### 4a. Add `current_file_override` local variable

At the top of `preprocess_internal`, alongside `current_line`, declare:

```c
int   current_line          = 1;
char *current_file_override = NULL;  /* non-NULL when #line overrides __FILE__ */
```

Add a `free(current_file_override);` before each `exit(1)` call and before
the final `return` in `preprocess_internal`.  (For the error-exit paths,
`current_file_override` may be NULL — `free(NULL)` is a no-op in C99.)

### 4b. Update `__FILE__` expansion to respect `current_file_override`

In the identifier expansion section, replace the `__FILE__` handler:

**Before:**
```c
if (id_len == 8 && strncmp(s + id_start, "__FILE__", 8) == 0) {
    gbuf_push(&out, '"');
    for (const char *fp = source_path; fp && *fp; fp++) {
        if (*fp == '"' || *fp == '\\') gbuf_push(&out, '\\');
        gbuf_push(&out, *fp);
    }
    gbuf_push(&out, '"');
}
```

**After:**
```c
if (id_len == 8 && strncmp(s + id_start, "__FILE__", 8) == 0) {
    const char *fname = current_file_override ? current_file_override : source_path;
    gbuf_push(&out, '"');
    for (const char *fp = fname; fp && *fp; fp++) {
        if (*fp == '"' || *fp == '\\') gbuf_push(&out, '\\');
        gbuf_push(&out, *fp);
    }
    gbuf_push(&out, '"');
}
```

### 4c. Add `#line` handler

In `preprocess_internal`, add the `#line` handler immediately before the
`#pragma` handler (i.e., the second-to-last handler before the unsupported
error):

```c
/* #line digit-sequence ["filename"] */
if (strncmp(s + in, "line", 4) == 0 &&
    !isalnum((unsigned char)s[in + 4]) && s[in + 4] != '_') {
    in += 4;
    while (s[in] == ' ' || s[in] == '\t') in++;
    if (!isdigit((unsigned char)s[in])) {
        fprintf(stderr, "error: #line requires a positive integer\n");
        free(out.data); free(spliced); free(current_file_override); exit(1);
    }
    long new_line = 0;
    while (isdigit((unsigned char)s[in]))
        new_line = new_line * 10 + (s[in++] - '0');
    /* The \n ending this directive line will increment current_line by 1,
     * so set it to new_line - 1 now so it reaches new_line after the \n. */
    current_line = (int)(new_line - 1);
    while (s[in] == ' ' || s[in] == '\t') in++;
    /* Optional filename string */
    if (s[in] == '"') {
        in++; /* skip opening '"' */
        GrowBuf fbuf;
        gbuf_init(&fbuf, 32);
        while (s[in] && s[in] != '"' && s[in] != '\n') {
            if (s[in] == '\\' && s[in + 1]) {
                in++;
                gbuf_push(&fbuf, s[in]);
            } else {
                gbuf_push(&fbuf, s[in]);
            }
            in++;
        }
        if (s[in] == '"') in++;
        gbuf_push(&fbuf, '\0');
        free(current_file_override);
        current_file_override = fbuf.data; /* fbuf.data now owned here */
    }
    while (s[in] && s[in] != '\n') in++; /* consume rest of line */
    continue;
}
```

**Notes:**
- `#line` affects `__LINE__` and `__FILE__` macro expansions in subsequent
  source text.  It does **not** update the `\x01` location markers emitted
  for `#include` transitions (those are based on the real file structure and
  are used by the lexer for error messages).  This is acceptable: `#line`
  is primarily used in generated source files, not in hand-written code where
  error-message accuracy matters.
- `current_line` is an `int`; truncation from `long` is benign for any
  realistic line count.
- The GrowBuf `fbuf` field `fbuf.data` is transferred into `current_file_override`
  and must not be separately freed.

---

## Task 5 — Add null-directive handling (`src/preprocessor.c`)

In `preprocess_internal`, add a null-directive check immediately **before**
the `#line` handler (or equivalently, at the very start of the directive
dispatch, immediately after the `!emitting` skip guard, before `#include`):

Actually, the cleanest placement is just before the "unsupported" error, as the
final fallthrough check:

**Before:**
```c
/* All other directives are unsupported. */
fprintf(stderr, "error: unsupported preprocessor directive\n");
free(out.data);
free(spliced);
exit(1);
```

**After:**
```c
/* Null directive: '#' followed by optional whitespace then newline — C99 §6.10.7. */
if (s[in] == '\n' || s[in] == '\0')
    continue;

/* All other directives are unsupported. */
fprintf(stderr, "error: unsupported preprocessor directive\n");
free(out.data);
free(spliced);
exit(1);
```

---

## Task 6 — Add `_Pragma` operator (`src/preprocessor.c`)

In the identifier expansion section of `preprocess_internal`, add a
`_Pragma` branch alongside the existing `__FILE__` and `__LINE__` checks.
Insert it as a third `else if` within the `if (emitting)` block:

```c
} else if (id_len == 7 && strncmp(s + id_start, "_Pragma", 7) == 0) {
    /* C99 §6.10.9: _Pragma("string") — destringize string, apply as pragma,
     * replace the entire _Pragma(...) expression with nothing. */
    size_t peek = in;
    while (s[peek] == ' ' || s[peek] == '\t') peek++;
    if (s[peek] != '(') {
        fprintf(stderr, "error: expected '(' after _Pragma\n");
        free(out.data); free(spliced); free(current_file_override); exit(1);
    }
    in = peek + 1; /* skip '(' */
    while (s[in] == ' ' || s[in] == '\t') in++;
    if (s[in] != '"') {
        fprintf(stderr, "error: _Pragma argument must be a string literal\n");
        free(out.data); free(spliced); free(current_file_override); exit(1);
    }
    in++; /* skip opening '"' */
    /* Collect destringized content: \" → ", \\ → \ */
    GrowBuf pbuf;
    gbuf_init(&pbuf, 32);
    while (s[in] && s[in] != '"' && s[in] != '\n') {
        if (s[in] == '\\' && s[in + 1] == '"') {
            gbuf_push(&pbuf, '"');
            in += 2;
        } else if (s[in] == '\\' && s[in + 1] == '\\') {
            gbuf_push(&pbuf, '\\');
            in += 2;
        } else {
            gbuf_push(&pbuf, s[in++]);
        }
    }
    if (s[in] == '"') in++;
    while (s[in] == ' ' || s[in] == '\t') in++;
    if (s[in] != ')') {
        fprintf(stderr, "error: expected ')' after _Pragma string\n");
        free(pbuf.data); free(out.data); free(spliced);
        free(current_file_override); exit(1);
    }
    in++; /* skip ')' */
    gbuf_push(&pbuf, '\0');
    /* Apply the pragma content: only 'once' is recognized. */
    if (strcmp(pbuf.data, "once") == 0 && source_path)
        macro_table_add_once(macros, source_path);
    /* else: silently ignore unknown _Pragma string */
    free(pbuf.data);
    /* _Pragma(...) is replaced by nothing (zero preprocessing tokens). */
}
```

**Notes:**
- `_Pragma` is handled only in `preprocess_internal`'s identifier expansion
  section, not in `expand_macros_text`.  This covers `_Pragma` appearing
  directly in source text.  `_Pragma` inside macro replacement text (an edge
  case) is not handled in this stage.
- The `current_file_override` must be freed on each error-exit path inside
  this block.

---

## Task 7 — Add `__func__` predefined identifier (`include/parser.h`, `src/parser.c`)

### 7a. Add `current_func_name` to `Parser` struct (`include/parser.h`)

Add a field immediately after `current_func_is_variadic`:

```c
    /* Stage 75-03: set while parsing the body of a variadic function
     * definition so __builtin_va_start can verify its context. */
    int current_func_is_variadic;
    /* Stage 105: set to the enclosing function name while parsing a function
     * body, so __func__ can be resolved to a string literal. */
    const char *current_func_name;
```

### 7b. Initialize in `parser_init` (`src/parser.c`)

In `parser_init`, add alongside the existing `current_func_is_variadic = 0`
initialization:

```c
parser->current_func_name = NULL;
```

### 7c. Set/restore around `parse_block` in function definition parsing (`src/parser.c`)

In the function definition arm (around line 3911), extend the existing
save/restore block:

**Before:**
```c
int saved_variadic = parser->current_func_is_variadic;
parser->current_func_is_variadic = func->is_variadic;
ast_add_child(func, parse_block(parser));
parser->current_func_is_variadic = saved_variadic;
```

**After:**
```c
int         saved_variadic  = parser->current_func_is_variadic;
const char *saved_func_name = parser->current_func_name;
parser->current_func_is_variadic = func->is_variadic;
parser->current_func_name        = d.name;
ast_add_child(func, parse_block(parser));
parser->current_func_is_variadic = saved_variadic;
parser->current_func_name        = saved_func_name;
```

`d.name` is a pointer into lexer-owned storage (set when the declarator was
parsed), so no copy is needed.

### 7d. Handle `__func__` in `parse_primary` (`src/parser.c`)

In `parse_primary`, add a check for the identifier `__func__` before the
general identifier dispatch.  A good placement is immediately before or after
the `TOKEN_IDENTIFIER` case that handles variable references, enum constants,
and built-ins.  Add it within the `TOKEN_IDENTIFIER` block (or as a new
`if`-check before it):

```c
/* C99 §6.4.2.2: __func__ predefined identifier — the name of the
 * lexically-enclosing function as a static const char array. */
if (parser->current.type == TOKEN_IDENTIFIER &&
    strcmp(parser->current.value, "__func__") == 0) {
    if (!parser->current_func_name)
        PARSER_ERROR(parser,
            "error: '__func__' used outside of a function body\n");
    parser->current = lexer_next_token(parser->lexer);
    const char *fname = parser->current_func_name;
    int len = (int)strlen(fname);
    /* Synthesize an AST_STRING_LITERAL identical to how a quoted string
     * literal is parsed: node->value points to the decoded bytes,
     * byte_length is the content length (not including the implicit NUL),
     * full_type is char[len+1]. */
    ASTNode *node = parser_node(parser, AST_STRING_LITERAL,
                                lexer_store_bytes(parser->lexer,
                                                  fname, (size_t)len));
    node->byte_length = len;
    node->decl_type   = TYPE_ARRAY;
    node->full_type   = type_array(type_char(), len + 1);
    return node;
}
```

**Notes:**
- `lexer_store_bytes` interns the function name bytes into the lexer's string
  pool, matching the storage convention for all `node->value` strings.
- The synthesized node is indistinguishable from a string literal containing
  the function name; it decays to `const char *` in expression contexts and is
  emitted as a `.rodata` string label in codegen, which is exactly the C99
  semantics of `__func__`.
- `parser->current_func_name` is `NULL` at file scope (initialized in
  `parser_init`, restored after each function body); a use of `__func__`
  outside any function body is therefore an error, which this check enforces.
- `strlen` is available via `<string.h>`, already included in `src/parser.c`.

---

## Task 8 — `src/version.c`

Bump `VERSION_STAGE` to `"01050000"`.

---

## Implementation order

1. `src/preprocessor.c` — extend `MacroTable` (Task 1).
2. `src/preprocessor.c` — `#pragma once` check in `preprocess_file` (Task 2).
3. `src/preprocessor.c` — `#pragma` handler (Task 3).
4. `src/preprocessor.c` — `#line` handler with `current_file_override` (Task 4).
5. `src/preprocessor.c` — null-directive check (Task 5).
6. `src/preprocessor.c` — `_Pragma` operator in identifier expansion (Task 6).
7. `include/parser.h` and `src/parser.c` — `__func__` (Task 7).
8. `src/version.c` — bump stage (Task 8).
9. Tests.
10. Documentation.

---

## Out of scope — do NOT do these in this stage

- **`#elifdef` / `#elifndef`** — these are C23, not C99; defer to a potential
  future C23 stage.
- **`__VA_OPT__`** — a C20 extension; out of scope.
- **Named variadic parameters** (`args...` syntax) — a GCC extension, not C99.
- **`_Pragma` inside macro replacement text** — an edge case not needed for
  practical C99 code; not handled in `expand_macros_text`.
- **`#pragma STDC` semantic effects** — C99 defines `STDC FP_CONTRACT`,
  `STDC FENV_ACCESS`, and `STDC CX_LIMITED_RANGE`; all are silently ignored
  since ClaudeC99 has no floating-point support.
- **`#include MACRO`** — computed include filenames where the argument is a
  macro expression rather than a literal `"file"` or `<file>`.
- **`#line` affecting `\x01` location markers** — `#line` overrides `__LINE__`
  and `__FILE__` expansion only; the lexer's line-tracking used for error
  messages is not affected.
- **`__func__` in file-scope initializers** — `__func__` is a block-scope
  predefined identifier; using it at file scope is an error (enforced by
  `parser->current_func_name == NULL` check).
- **`__FUNCTION__` / `__PRETTY_FUNCTION__`** — GCC extensions, not C99.

---

## Bootstrap caveats

The compiler's own source (`src/*.c`, `include/*.h`) does not use `#pragma`,
`_Pragma`, `#line`, bare `#`, or `__func__`.  No changes to the compiler's
own source are required.  The self-host cycle (`./build.sh --mode=self-host`)
should pass cleanly.

Run `./build.sh --mode=self-host` before declaring the stage complete.

---

## Tests

### Valid — `#pragma` ignored

```c
/* test_pragma_unknown_ignored__0.c */
#pragma GCC optimize("O2")
#pragma pack(push, 1)
int main(void) { return 0; }
```
Expected: exit 0 (unknown pragmas silently ignored).

### Valid — `#pragma once` prevents double inclusion

Use an integration test with a helper header.

`test/integration/test_pragma_once/main.c`:
```c
#include "helper.h"
#include "helper.h"  /* second inclusion: should be a no-op */
int main(void) { return VALUE - 42; }  /* expect 0 */
```

`test/integration/test_pragma_once/helper.h`:
```c
#pragma once
#define VALUE 42
```

Without `#pragma once`, the second `#include "helper.h"` would redefine
`VALUE` (harmlessly) and, in a more complex case, redefine structs, causing
errors.  This test verifies the file is only processed once.

### Valid — `#line` changes `__LINE__`

```c
/* test_line_directive_line_number__0.c
 * Note: This is a print-tokens test to avoid needing printf.
 * Instead, verify via a direct computation: */
int main(void) {
    int before = __LINE__;
#line 100
    int after = __LINE__;
    return (before < 100 && after == 100) ? 0 : 1;
}
```
Expected: exit 0.

### Valid — `#line` changes both `__LINE__` and `__FILE__`

```c
/* test_line_directive_file_and_line__0.c */
#line 1 "virtual.c"
int main(void) {
    /* __FILE__ is now "virtual.c", __LINE__ is 2 */
    return (__LINE__ == 2) ? 0 : 1;
}
```
Expected: exit 0.

### Valid — Null directive

```c
/* test_null_directive__0.c */
#
int main(void) {
#
    return 0;
#
}
```
Expected: exit 0.

### Valid — `_Pragma` ignored (non-once)

```c
/* test_pragma_operator_ignored__0.c */
_Pragma("GCC diagnostic ignored \"-Wunused\"")
int main(void) { return 0; }
```
Expected: exit 0.

### Valid — `_Pragma("once")` acts as `#pragma once`

Integration test:

`test/integration/test_pragma_operator_once/main.c`:
```c
#include "guarded.h"
#include "guarded.h"
int main(void) { return FLAG - 1; }  /* expect 0 */
```

`test/integration/test_pragma_operator_once/guarded.h`:
```c
_Pragma("once")
#define FLAG 1
```

### Valid — `__func__` in a simple function

```c
/* test_func_predefined_identifier__0.c */
#include <string.h>
int check(void) {
    return strcmp(__func__, "check");  /* expect 0 */
}
int main(void) { return check(); }
```
Expected: exit 0.

### Valid — `__func__` in multiple functions

```c
/* test_func_predefined_identifier_multi__0.c */
#include <string.h>
int foo(void) { return strcmp(__func__, "foo"); }
int bar(void) { return strcmp(__func__, "bar"); }
int main(void) { return foo() + bar(); }
```
Expected: exit 0.

### Invalid — `__func__` at file scope

```c
/* test_invalid_func_at_file_scope__used_outside_of_a_function_body.c */
const char *name = __func__;
int main(void) { return 0; }
```
Expected error contains: `used outside of a function body`

---

## Docs (at stage close, after all tests pass)

- **`README.md`** — update "Through stage …" line; note that `#pragma` (with
  `#pragma once`), `_Pragma`, `#line`, null directives, and `__func__` are now
  supported; refresh test totals.
- **`docs/grammar.md`** — no new grammar productions; note `__func__` in the
  predefined-identifiers section if one exists, or add a brief note under
  preprocessing.
- Run the **`update-supplemental-docs`** skill.
- **`docs/self-compilation-report.md`** — record the stage-105 self-host run.
