# ClaudeC99 Stage 107 — `inline` keyword, `<assert.h>`, `va_copy` codegen

## Goal

Close three small but frequently-hit C99 gaps that are independent of
floating-point support:

1. **`inline` function specifier** — currently causes a parse error.  Fix:
   tokenize `inline` and consume it silently in declaration specifiers
   (parse-and-ignore, same pattern as `restrict` in Stage 106).

2. **`<assert.h>` stub** — the header does not exist.  Fix: add
   `test/include/assert.h` with the `assert` macro using the preprocessor
   features already present (`#expr` stringification, `__FILE__`, `__LINE__`,
   `fprintf`, `abort`).

3. **`va_copy` codegen** — `va_copy(dst, src)` currently generates no code at
   all (silent no-op stub since Stage 75-03).  Correct behaviour requires
   copying the 24-byte `va_list` struct from `src` to `dst` on the stack.

All three changes are self-contained.  No new AST nodes and no grammar
productions change.

---

## Background — current gaps

### Gap 1 — `inline` keyword causes a hard parse error

`inline` is not in the lexer's keyword table.  An unrecognised identifier at
the start of a declaration falls through to the "expected type specifier"
error:

```c
inline int square(int x) { return x * x; }   /* error: expected type specifier */
static inline void noop(void) {}              /* error: expected type specifier */
```

System headers and real C99 code use `inline` pervasively.  The fix is
identical to what Stage 106 did for `restrict`: add a token, consume and
discard it wherever function specifiers may appear.

### Gap 2 — `<assert.h>` does not exist

```c
#include <assert.h>
assert(x > 0);   /* fatal: cannot open assert.h */
```

The `assert` macro is one of the most common C idioms.  The preprocessor
already supports everything needed to implement it: `#` stringification,
`__FILE__`, `__LINE__`, `fprintf`, and `abort`.

### Gap 3 — `va_copy` is a silent no-op

The codegen stub at `src/codegen.c:3971` treats `AST_BUILTIN_VA_COPY`
identically to `va_end` — it sets `result_type = TYPE_VOID` and returns
without emitting any instructions.  A program that copies a `va_list` and
uses the copy will read uninitialised stack bytes:

```c
void vprint_twice(const char *fmt, va_list ap) {
    va_list copy;
    va_copy(copy, ap);   /* no-op: copy is uninitialised */
    vprintf(fmt, ap);
    vprintf(fmt, copy);  /* reads garbage */
    va_end(copy);
}
```

The SysV AMD64 ABI `va_list` is a 24-byte struct laid out at a known stack
offset; a correct `va_copy` copies those three 8-byte words.

---

## C99 references

| Feature | C99 section |
|---------|-------------|
| `inline` function specifier | §6.7.4 |
| `<assert.h>` / `assert` macro | §7.2 |
| `va_copy` macro | §7.15.1 |
| SysV AMD64 ABI `va_list` layout | ABI §3.5.7 |

---

## Task 1 — `inline` keyword (`src/lexer.c`, `src/parser.c`)

### 1a. Lexer — add `TOKEN_INLINE` (`src/lexer.c`)

In `include/token.h`, add `TOKEN_INLINE` to the token enum, adjacent to the
other keyword tokens (`TOKEN_EXTERN`, `TOKEN_STATIC`):

```c
TOKEN_INLINE,
```

In `src/lexer.c`, in the keyword recognition block (the chain of
`else if (strcmp(...))` branches), add an entry for `inline` parallel to the
existing `restrict` entry:

```c
else if (strcmp(ident_buf.data, "inline") == 0) {
    token.type = TOKEN_INLINE;
    token.value = "inline";
    token.value_len = 6;
}
```

Also add `TOKEN_INLINE` to the `token_type_name()` display table:

```c
case TOKEN_INLINE: return "'inline'";
```

### 1b. Parser — consume `TOKEN_INLINE` in declaration specifiers (`src/parser.c`)

`inline` is a C99 _function-specifier_ (§6.7.4).  It may appear before or
after any storage-class specifier or type qualifier in a declaration:

```c
inline int f(void);
static inline int g(void);
extern inline int h(void);
inline static int k(void);
```

The one-stop location is the `while (1)` loop inside
`parse_declaration_specifiers` (`src/parser.c:3447`).  Add a new branch that
consumes `TOKEN_INLINE` and discards it, alongside `TOKEN_CONST` and
`TOKEN_VOLATILE`:

```c
} else if (parser->current.type == TOKEN_INLINE) {
    /* C99 §6.7.4 function-specifier: parse and ignore (no codegen effect) */
    parser->current = lexer_next_token(parser->lexer);
```

Place this branch immediately after the `TOKEN_VOLATILE` branch and before the
`TOKEN_EXTERN` / `TOKEN_STATIC` / `TOKEN_TYPEDEF` branch.

**No other parser changes are needed.**  `inline` never appears inside a
declarator (unlike `restrict`, which appears after `*`), so pointer-declarator
loops do not need to be updated.

**Semantics note**: `inline` hints to the compiler that the function body may
be inlined at call sites.  ClaudeC99 does not perform inlining; parse-and-ignore
is the correct stub behaviour, consistent with how `volatile` and `restrict`
are handled.

---

## Task 2 — `<assert.h>` stub (`test/include/assert.h`)

Create a new file `test/include/assert.h`:

```c
#ifndef CLAUDEC99_ASSERT_H
#define CLAUDEC99_ASSERT_H

#ifdef NDEBUG
#define assert(expr) ((void)0)
#else
#include <stdio.h>
#include <stdlib.h>
#define assert(expr) \
    ((expr) ? (void)0 : (fprintf(stderr, \
        "assertion failed: %s, file %s, line %d\n", \
        #expr, __FILE__, __LINE__), abort(), (void)0))
#endif

#endif
```

**Design notes:**

- The `NDEBUG` guard follows C99 §7.2: when `NDEBUG` is defined, `assert`
  must expand to a void expression that evaluates its argument zero times.
  `((void)0)` satisfies this requirement.

- `#expr` uses the preprocessor's existing `#`-operator stringification
  (Stage 57-04 feature, present since Stage 57).

- `__FILE__` and `__LINE__` are supported by the preprocessor
  (`src/preprocessor.c:1788`).

- The comma expression `(fprintf(...), abort(), (void)0)` ensures that:
  1. The error message is printed before aborting.
  2. The entire expression has type `void`.
  3. The `(void)0` after `abort()` suppresses any "control reaches end of
     non-void expression" concern (though `abort()` is `_Noreturn` in standard
     C; we don't model that yet).

- `fprintf` and `abort` resolve against the already-present stubs in
  `test/include/stdio.h` and `test/include/stdlib.h`.

- Including `<stdio.h>` and `<stdlib.h>` from inside `<assert.h>` is safe
  because both headers are guarded by their own `#ifndef` include guards.

---

## Task 3 — `va_copy` codegen (`src/codegen.c`)

### Background — `va_list` memory layout

Per `test/include/stdarg.h`, `va_list` is defined as an array of one
`__claudec00_va_list_tag` struct:

```c
typedef struct __claudec00_va_list_tag {
    unsigned int gp_offset;       /* bytes  0– 3: next GP reg index × 8  */
    unsigned int fp_offset;       /* bytes  4– 7: next FP reg index × 8  */
    void *overflow_arg_area;      /* bytes  8–15: pointer into caller frame */
    void *reg_save_area;          /* bytes 16–23: pointer to saved-reg area */
} va_list[1];
```

Total size: 24 bytes.  For a local variable `ap`, the codegen stores it at
`[rbp - ap_off]` through `[rbp - ap_off + 23]` where `ap_off` is
`lv_ap->offset`.  The three 8-byte words are at:

| Word | Address in NASM notation |
|------|--------------------------|
| gp_offset ∥ fp_offset | `[rbp - ap_off]`      |
| overflow_arg_area      | `[rbp - (ap_off - 8)]`  |
| reg_save_area          | `[rbp - (ap_off - 16)]` |

### 3a. Split `va_end` and `va_copy` in the no-op branch

In `src/codegen.c`, locate the combined no-op branch (currently at ~line 3971):

```c
if (node->type == AST_BUILTIN_VA_END ||
    node->type == AST_BUILTIN_VA_COPY) {
    node->result_type = TYPE_VOID;
    return;
}
```

Split it into two separate blocks — keep `va_end` as a no-op and replace the
`va_copy` no-op with the real implementation:

```c
if (node->type == AST_BUILTIN_VA_END) {
    node->result_type = TYPE_VOID;
    return;
}
if (node->type == AST_BUILTIN_VA_COPY) {
    /* children[0] = dst (va_list), children[1] = src (va_list) */
    ASTNode *dst_node = node->children[0];
    ASTNode *src_node = node->children[1];
    LocalVar *lv_dst = codegen_find_var(cg, dst_node->value);
    LocalVar *lv_src = codegen_find_var(cg, src_node->value);
    if (!lv_dst || !lv_src) {
        compile_error("error: va_copy: operand is not a local variable\n");
    }
    int dst_off = lv_dst->offset;
    int src_off = lv_src->offset;
    /* Copy 24-byte va_list struct: three 8-byte moves via rax. */
    fprintf(cg->output, "    mov rax, [rbp - %d]\n",      src_off);
    fprintf(cg->output, "    mov [rbp - %d], rax\n",      dst_off);
    fprintf(cg->output, "    mov rax, [rbp - %d]\n",      src_off - 8);
    fprintf(cg->output, "    mov [rbp - %d], rax\n",      dst_off - 8);
    fprintf(cg->output, "    mov rax, [rbp - %d]\n",      src_off - 16);
    fprintf(cg->output, "    mov [rbp - %d], rax\n",      dst_off - 16);
    node->result_type = TYPE_VOID;
    return;
}
```

**Why rax is safe here**: `va_copy` appears only in expression statements
(its result is `void`); no caller expects a value in rax after the call, and
the generated instructions do not interfere with a surrounding expression
context.

**Correctness check**: after the copy `dst[0]` is an independent snapshot of
`src[0]`.  Subsequent `va_arg` calls on `dst` advance `dst->gp_offset`
independently from `src->gp_offset`, which is the required semantics.
`reg_save_area` points into the variadic function's own frame and is valid for
the function's lifetime, so the pointer copy is correct.

---

## Task 4 — Version bump (`src/version.c`)

Change:

```c
#define VERSION_STAGE  "01060000"
```

to:

```c
#define VERSION_STAGE  "01070000"
```

---

## Implementation order

1. `include/token.h` — add `TOKEN_INLINE` (Task 1a)
2. `src/lexer.c` — recognize `"inline"` → `TOKEN_INLINE`; add to display table (Task 1a)
3. `src/parser.c` — consume `TOKEN_INLINE` in `parse_declaration_specifiers` (Task 1b)
4. `src/codegen.c` — split va_end/va_copy no-op; implement va_copy 24-byte copy (Task 3a)
5. `test/include/assert.h` — new file (Task 2)
6. `src/version.c` — bump stage to `01070000` (Task 4)
7. Tests (see below)
8. Build, full test suite, self-host cycle

---

## Out of scope — do NOT do these in this stage

- **`inline` codegen** — actual function inlining requires data-flow analysis
  and is out of scope.  Parse-and-ignore is the correct stub.
- **`inline` linkage semantics** — C99 §6.7.4 defines special external/internal
  linkage rules for `inline` definitions.  These are not enforced; all
  functions continue to have their declared linkage.
- **`static_assert` / `_Static_assert`** — a C11 feature; deferred.
- **`_Noreturn`** — a C11 keyword; deferred.
- **`__attribute__`** — a GCC extension; deferred.
- **`<math.h>`** — requires floating-point support; deferred.
- **`<locale.h>`, `<signal.h>`, `<iso646.h>`** — deferred.
- **Implicit return in void functions** — already implemented at
  `src/codegen.c:5380`; the checklist item is stale and will be ticked as
  part of this stage's documentation update with no code change required.
- **`va_arg` for floating-point types** — deferred until FP support.
- **`va_copy` for `va_list` passed as a pointer parameter** — only local
  `va_list` variables are supported; a `va_list *` argument is out of scope.

---

## Tests

### Valid — `inline` function, file scope

```c
/* test_inline_func__0.c */
inline int square(int x) { return x * x; }
int main(void) { return square(5) - 25; }
```
Expected exit: 0.

### Valid — `static inline` function

```c
/* test_static_inline__0.c */
static inline int add(int a, int b) { return a + b; }
int main(void) { return add(20, 22) - 42; }
```
Expected exit: 0.

### Valid — `extern inline` declaration

```c
/* test_extern_inline__0.c */
extern inline int id(int x);
inline int id(int x) { return x; }
int main(void) { return id(0); }
```
Expected exit: 0.

### Valid — `assert` passes

```c
/* test_assert_pass__0.c */
#include <assert.h>
int main(void) {
    assert(1 == 1);
    assert(2 + 2 == 4);
    return 0;
}
```
Expected exit: 0.

### Valid — `assert` with `NDEBUG` (no-op)

```c
/* test_assert_ndebug__0.c */
#define NDEBUG
#include <assert.h>
int main(void) {
    assert(0);   /* would fail without NDEBUG */
    return 0;
}
```
Expected exit: 0.

### Valid — `va_copy` basic

```c
/* test_va_copy_basic__0.c */
#include <stdarg.h>
#include <stdio.h>

static int sum(int n, ...) {
    va_list ap, cp;
    va_start(ap, n);
    va_copy(cp, ap);
    int s1 = 0, s2 = 0;
    int i;
    for (i = 0; i < n; i++) s1 += va_arg(ap, int);
    for (i = 0; i < n; i++) s2 += va_arg(cp, int);
    va_end(ap);
    va_end(cp);
    return (s1 == s2) ? s1 : -1;
}

int main(void) {
    return sum(3, 10, 11, 21) - 42;
}
```
Expected exit: 0.

### Valid — implicit void return (regression / checklist close)

```c
/* test_void_implicit_return__0.c */
static int g_val = 0;
void set(int v) { g_val = v; }   /* no explicit return */
int main(void) {
    set(42);
    return g_val - 42;
}
```
Expected exit: 0.  (Confirms `src/codegen.c:5380` handles fall-off-end.)

### Invalid — `assert` failure aborts (exit non-zero)

This test verifies that `assert(0)` terminates the process.  Use the existing
invalid-test infrastructure: the compiler must succeed (no compile error), the
assembled binary must exit non-zero.

```c
/* test_assert_fail__abort.c  (invalid suite — expected exit non-zero) */
#include <assert.h>
int main(void) {
    assert(0);
    return 0;
}
```
Expected: compile succeeds; runtime exits with a non-zero status.

> **Note on invalid-suite placement**: the existing `test/invalid/` runner
> checks for a compiler error message.  A test that compiles cleanly but fails
> at runtime does not fit that suite.  Place `test_assert_fail` in
> `test/valid/` with a non-zero expected exit suffix, e.g.
> `test_assert_fail__134.c` (SIGABRT raises signal 6; shell exit status is
> 128 + 6 = 134).  Alternatively, if the exit code is not stable across
> platforms, use an integration test.

---

## Docs (at stage close, after all tests pass)

- **`README.md`** — update "Through stage …" line; note `inline` is now
  parsed, `<assert.h>` is available, `va_copy` is functional; refresh test
  totals.
- **`docs/grammar.md`** — add `inline` to the function-specifier section of
  the declaration grammar.
- **`docs/outlines/checklist.md`** — add Stage 107 section; tick `inline`,
  `<assert.h>`, `va_copy`, and `Implicit return in void functions`.
- Run the **`update-supplemental-docs`** skill to refresh the feature
  checklist, parse-tree, and status/features snapshots.
- **`docs/self-compilation-report.md`** — record the stage-107 self-host run.
