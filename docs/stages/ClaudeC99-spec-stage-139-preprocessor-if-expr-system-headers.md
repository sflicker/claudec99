# ClaudeC99 Stage 139 — Preprocessor `#if` Expression Gaps Blocking System Headers

## Overview

Three deficiencies in the `#if`/`#elif` constant-expression evaluator
(`eval_cond_primary` and friends in `src/preprocessor.c`) prevent the compiler
from parsing system headers such as `/usr/include/features.h` and the glibc
include chain. With the project's stub headers in `test/include/` the full
test suite (88 tests) passes today. Running the same suite against the real
system headers yields 31 passing and 53 failing, all failures traceable to one
of three preprocessor gaps.

This stage fixes all three gaps so that any test whose only obstacle is a
system-header parse failure will start passing when `test/include` is
substituted with the GCC system include paths.

---

## Root-Cause Summary

| ID | Gap | Trigger in system headers | Error produced |
|----|-----|--------------------------|----------------|
| PP-01 | Integer literal suffixes (`L`, `U`, `LL`, `UL`, …) unconsumed | `features.h` line 243: `__STDC_VERSION__ > 201710L` | `expected ')' in preprocessor expression` |
| PP-02 | Function-like macros unsupported inside `#if` expressions | `sys/cdefs.h` line 153: `__glibc_clang_prereq (9, 0)` etc. | `unsupported #if expression` |
| PP-03 | Ternary operator `?:` unsupported in `#if` expressions | `features.h` line 440: `defined __cplusplus ? … : …` | `extra tokens after #if expression` |

All three errors call `exit(1)`, so a single failing directive aborts the
entire compilation.

---

## PP-01: Integer Literal Suffixes Left Unconsumed

### Root Cause

`eval_cond_primary` (line ~825, `src/preprocessor.c`) reads decimal digits
with a plain `while (isdigit(...))` loop and returns immediately. Suffix
characters (`L`, `U`, `LL`, `UL`, `ULL`, `lu`, etc.) are left in the input
stream. When the expression containing the literal is wrapped in parentheses
the paren-group closer reads the leftover suffix character instead of `)` and
dies:

```c
/* current code */
if (isdigit((unsigned char)s[*in])) {
    long value = 0;
    while (isdigit((unsigned char)s[*in]))
        value = value * 10 + (s[(*in)++] - '0');
    return value;          /* suffix still at s[*in] */
}
```

### Required Change — `src/preprocessor.c`, `eval_cond_primary`

After the digit-reading loop, consume and discard any trailing integer-suffix
characters before returning:

```c
if (isdigit((unsigned char)s[*in])) {
    long value = 0;
    /* hex literal */
    if (s[*in] == '0' && (s[*in + 1] == 'x' || s[*in + 1] == 'X')) {
        *in += 2;
        while (isxdigit((unsigned char)s[*in]))
            value = value * 16 + (isdigit((unsigned char)s[*in])
                    ? s[(*in)++] - '0'
                    : tolower((unsigned char)s[(*in)++]) - 'a' + 10);
    } else {
        while (isdigit((unsigned char)s[*in]))
            value = value * 10 + (s[(*in)++] - '0');
    }
    /* consume integer-suffix: u/U, l/L, ll/LL, ul/UL, ull/ULL, etc. */
    while (s[*in] == 'u' || s[*in] == 'U' ||
           s[*in] == 'l' || s[*in] == 'L')
        (*in)++;
    return value;
}
```

Hex support is included because system headers occasionally use hex constants
in `#if` guards (e.g., `_POSIX_C_SOURCE >= 0x600L`).

---

## PP-02: Function-Like Macros Unsupported in `#if` Expressions

### Root Cause

When `eval_cond_primary` encounters an identifier that resolves to a
function-like macro (one with `param_count >= 0`) it immediately errors:

```c
MacroDef *m = macro_find(macros, s + name_start, name_len);
if (!m) return 0L;
if (m->param_count != -1) {
    fprintf(stderr, "error: unsupported #if expression\n");
    free(out_data); free(spliced_buf); exit(1);
}
```

glibc headers rely heavily on function-like macros inside `#if`:

```c
/* sys/cdefs.h */
#if __USE_FORTIFY_LEVEL == 3 && (__glibc_clang_prereq (9, 0) \
                                 || __GNUC_PREREQ (12, 0))
#if __GNUC_PREREQ (2,96) || __glibc_has_attribute (__malloc__)
```

```c
/* string.h */
#if defined __USE_MISC || defined __USE_XOPEN || __GLIBC_USE (ISOC2X)
```

The standard solution used by all real C preprocessors is to fully
macro-expand the `#if` condition text before evaluating it as a constant
expression.

### Required Change — `src/preprocessor.c`, `#if` and `#elif` handlers

**Step 1.** In the `#if` handler (around line 1288) extract the raw condition
text, pass it through `expand_macros_text`, and evaluate the expanded result.
The current code evaluates inline against the raw source:

```c
/* current */
while (s[in] == ' ' || s[in] == '\t') in++;
long ifval = eval_cond_expr(s, &in, macros, out.data, spliced);
```

Change it to pre-expand first:

```c
while (s[in] == ' ' || s[in] == '\t') in++;
/* collect condition to end of (spliced) line */
size_t cond_start = in;
while (s[in] && s[in] != '\n') in++;
size_t cond_len = in - cond_start;
char *cond_text = malloc(cond_len + 1);
memcpy(cond_text, s + cond_start, cond_len);
cond_text[cond_len] = '\0';
char *expanded = expand_macros_text(cond_text, macros);
free(cond_text);
size_t exp_in = 0;
while (expanded[exp_in] == ' ' || expanded[exp_in] == '\t') exp_in++;
long ifval = eval_cond_expr(expanded, &exp_in, macros, out.data, spliced);
while (expanded[exp_in] == ' ' || expanded[exp_in] == '\t') exp_in++;
if (expanded[exp_in] != '\0') {
    fprintf(stderr, "error: extra tokens after #if expression\n");
    free(expanded); free(out.data); free(spliced); exit(1);
}
free(expanded);
```

**Step 2.** Apply the same pre-expansion pattern to every `#elif` handler.

**Step 3.** Because the evaluator now receives fully-expanded text, the
function-like macro guard in `eval_cond_primary` can be removed entirely: after
expansion a function-like call either produces a numeric token or another
identifier (which is then undefined and evaluates to 0).

**Important:** `expand_macros_text` already handles the `defined` operator
correctly through the macro table: `defined` is never placed in the macro
table, so it passes through the expander unchanged, and the `eval_cond_primary`
branch that handles `defined(X)` / `defined X` continues to work on the
expanded string. The identifiers inside `defined(X)` are not macro-expanded by
the evaluator — they are looked up directly, which is the correct C99
behaviour.

---

## PP-03: Ternary Operator `?:` Unsupported in `#if` Expressions

### Root Cause

The expression grammar in `src/preprocessor.c` tops out at
`eval_cond_logical_or`; there is no ternary level. `eval_cond_expr` simply
delegates to `eval_cond_logical_or` and returns. The `?` character is
therefore an unrecognised token left at the top level, triggering "extra tokens
after #if expression".

The ternary appears in glibc at:

```c
/* features.h line 440 */
#if defined __cplusplus ? __cplusplus >= 201402L : defined __USE_ISOC11
```

### Required Change — `src/preprocessor.c`

Add `eval_cond_ternary` between `eval_cond_logical_or` and `eval_cond_expr`.
The ternary has lower precedence than `||`:

```c
static long eval_cond_ternary(const char *s, size_t *in, MacroTable *macros,
                               char *out_data, char *spliced_buf) {
    long cond = eval_cond_logical_or(s, in, macros, out_data, spliced_buf);
    while (s[*in] == ' ' || s[*in] == '\t') (*in)++;
    if (s[*in] != '?') return cond;
    (*in)++;                        /* consume '?' */
    while (s[*in] == ' ' || s[*in] == '\t') (*in)++;
    long then_val = eval_cond_ternary(s, in, macros, out_data, spliced_buf);
    while (s[*in] == ' ' || s[*in] == '\t') (*in)++;
    if (s[*in] != ':') {
        fprintf(stderr, "error: expected ':' in ternary preprocessor expression\n");
        free(out_data); free(spliced_buf); exit(1);
    }
    (*in)++;                        /* consume ':' */
    while (s[*in] == ' ' || s[*in] == '\t') (*in)++;
    long else_val = eval_cond_ternary(s, in, macros, out_data, spliced_buf);
    return cond ? then_val : else_val;
}
```

Update `eval_cond_expr` to call `eval_cond_ternary` instead of
`eval_cond_logical_or`:

```c
static long eval_cond_expr(const char *s, size_t *in, MacroTable *macros,
                            char *out_data, char *spliced_buf) {
    return eval_cond_ternary(s, in, macros, out_data, spliced_buf);
}
```

The forward declarations at the top of the expression-evaluator block must be
updated to include `eval_cond_ternary`.

---

## Test Plan

### New preprocessor unit tests (integration suite)

Add the following tests under `test/integration/`. Each is a self-contained
`.c` file that exercises one of the three fixed cases; the exit code encodes
the expected result.

#### PP-01: Integer literal suffixes

**`test/integration/test_pp_if_L_suffix/test_pp_if_L_suffix.c`**
```c
int main(void) {
    /* 201710L > 199901L — both have L suffix */
#if 201710L > 199901L
    return 0;
#else
    return 1;
#endif
}
```
Exit code: `0` (success).

**`test/integration/test_pp_if_UL_suffix_in_parens/test_pp_if_UL_suffix_in_parens.c`**
```c
int main(void) {
    /* suffix inside parenthesised sub-expression — was the crash case */
#if (1UL << 31) > 0
    return 0;
#else
    return 1;
#endif
}
```
Exit code: `0`.

**`test/integration/test_pp_if_hex_constant/test_pp_if_hex_constant.c`**
```c
int main(void) {
#if 0x600 >= 0x200
    return 0;
#else
    return 1;
#endif
}
```
Exit code: `0`.

#### PP-02: Function-like macros in `#if`

**`test/integration/test_pp_if_funclike_macro/test_pp_if_funclike_macro.c`**
```c
#define ATLEAST(maj, min) ((maj) * 100 + (min) >= 402)

int main(void) {
#if ATLEAST(4, 6)
    return 0;
#else
    return 1;
#endif
}
```
Exit code: `0`.

**`test/integration/test_pp_if_funclike_token_paste/test_pp_if_funclike_token_paste.c`**
```c
#define CAT(a, b) a ## b
#define CAT_VAL_1 1
#define CAT_VAL_0 0

int main(void) {
    /* CAT(CAT_VAL, 1) expands to CAT_VAL_1 which is 1 */
#if CAT(CAT_VAL, 1)
    return 0;
#else
    return 1;
#endif
}
```
Exit code: `0`.

**`test/integration/test_pp_if_funclike_zero/test_pp_if_funclike_zero.c`**
```c
#define ALWAYS_ZERO(x) 0

int main(void) {
    /* function-like macro that always expands to 0 */
#if ALWAYS_ZERO(999)
    return 1;
#else
    return 0;
#endif
}
```
Exit code: `0`.

#### PP-03: Ternary operator in `#if`

**`test/integration/test_pp_if_ternary_true/test_pp_if_ternary_true.c`**
```c
#define FLAG 1

int main(void) {
#if FLAG ? 1 : 0
    return 0;
#else
    return 1;
#endif
}
```
Exit code: `0`.

**`test/integration/test_pp_if_ternary_false/test_pp_if_ternary_false.c`**
```c
#define FLAG 0

int main(void) {
    /* ternary picks the else branch; that branch is 7 (non-zero → #if true) */
#if FLAG ? 0 : 7
    return 0;
#else
    return 1;
#endif
}
```
Exit code: `0`.

**`test/integration/test_pp_if_ternary_defined/test_pp_if_ternary_defined.c`**
```c
/* Mirrors the features.h pattern exactly */
#define __cplusplus 0
#define __USE_ISOC11 1

int main(void) {
#if defined __cplusplus ? __cplusplus >= 201402L : defined __USE_ISOC11
    return 0;
#else
    return 1;
#endif
}
```
Exit code: `0` (`defined __cplusplus` is true so the ternary picks
`__cplusplus >= 201402L` which is `0 >= 201402L` = false; wait — see note
below).

> **Note:** In the actual glibc expression `defined __cplusplus` is true when
> compiling as C++, meaning the ternary selects the C++ branch. When our
> compiler processes this (not C++), `defined __cplusplus` is false, so the
> ternary falls through to `defined __USE_ISOC11`. In the test above `__cplusplus`
> is explicitly `#define`d to make `defined __cplusplus` true; then
> `0 >= 201402L` is false, so the `#if` is false and the `#else` branch runs —
> the expected exit code is **`1`**. Adjust the test or set `#define __USE_ISOC11 1`
> and remove `__cplusplus` to get exit code `0`.

Revised recommended test:

```c
/* No __cplusplus defined → ternary picks else-branch (defined __USE_ISOC11) */
#define __USE_ISOC11 1

int main(void) {
#if defined __cplusplus ? __cplusplus >= 201402L : defined __USE_ISOC11
    return 0;
#else
    return 1;
#endif
}
```
Exit code: `0`.

### System-header validation script

Add `test/integration/run_tests_sysinclude.sh` — a copy of `run_tests.sh`
with `DEFAULT_IFLAGS` replaced by the GCC system include search paths:

```sh
DEFAULT_IFLAGS=(
    "-I/usr/lib/gcc/x86_64-linux-gnu/13/include"
    "-I/usr/local/include"
    "-I/usr/include/x86_64-linux-gnu"
    "-I/usr/include"
)
```

This script is not part of the standard `build.sh` CI run (the stub headers
remain the default), but serves as a manual validation target to confirm that
the 43 tests that failed due to PP-01/PP-02/PP-03 now pass. Tests unrelated to
preprocessor gaps (the 10 other failures documented in the investigation) are
expected to remain failing under this script and are out of scope for this
stage.

---

## Version Bump

Change `VERSION_STAGE` in `src/version.c` from `"13800000"` to `"13900000"`.

---

## Implementation Order

1. `src/preprocessor.c` — consume integer suffixes and hex literals in
   `eval_cond_primary` (PP-01)
2. `src/preprocessor.c` — add `eval_cond_ternary` and wire it into
   `eval_cond_expr` (PP-03; independent of PP-02, simpler)
3. `src/preprocessor.c` — pre-expand the `#if`/`#elif` condition text through
   `expand_macros_text` before evaluation, and remove the function-like macro
   guard (PP-02)
4. `src/version.c` — version bump to `13900000`
5. Add the 8 new integration test directories listed above
6. Add `test/integration/run_tests_sysinclude.sh`
7. Run `./build.sh` — all 88 existing tests must still pass
8. Run `./test/integration/run_tests_sysinclude.sh` — confirm at least 77 of
   the 84 integration tests pass (the 42 failing due to PP-01/PP-02 and the
   4 failing due to PP-03 should now pass)
9. Self-host cycle: `./build.sh --mode=self-host`
10. Commit with `Co-Authored-By: Claude Sonnet 4.6 <noreply@anthropic.com>`

---

## Notes

- The pre-expansion approach for PP-02 is the correct standards-defined
  behaviour: C99 §6.10.1 requires that macro replacement be performed on the
  controlling expression before evaluation. `defined` is handled before
  expansion in the evaluator and is not placed in the macro table, so it
  correctly survives `expand_macros_text` unchanged.
- Short-circuit evaluation of `&&` and `||` (not currently implemented) would
  have masked PP-01 and PP-02 in many cases. The pre-expansion fix makes the
  lack of short-circuiting harmless for the typical system-header patterns.
- The 7 remaining failures with system headers after this stage are:
  `test_pp_angle_include` (test-specific `<add.h>` not in system path),
  `test_pp_include_recursive` (max depth with deep system include chains),
  `test_pp_include_missing` (intentional error test, unaffected),
  3 `werror` tests (`const_addr_discard_werror`, `const_ptr_discard_werror`,
  `struct_member_const_discard_werror` — const-discard interaction with the
  real `FILE *` typedef), and `test_max_errors_multi` (function signature
  mismatch unrelated to headers). None involve the preprocessor expression
  evaluator.
