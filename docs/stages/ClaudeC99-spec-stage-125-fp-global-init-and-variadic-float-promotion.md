# ClaudeC99 Stage 125 — FP Globals from Integer Initializers, Variadic Float Promotion, and Stale Checklist Cleanup

## Goal

Two correctness fixes for floating-point code, plus closing out stale checklist items:

| Task | Feature | Where |
|------|---------|--------|
| 1 | File-scope FP globals initialized from integer literals emit integer bits | `src/codegen.c` — `codegen_add_global` |
| 2 | `float` args not promoted to `double` in variadic calls | `src/codegen.c` — call arg emission |
| 3 | Stale checklist cleanup | `docs/outlines/checklist.md` |

---

## Background: What Was Already Done

The `docs/outlines/checklist.md` contains two unchecked items that are **already implemented** from Stage 110:

- **"Mixed integer/floating-point arithmetic"** — `if (fp_is_arith && (type_is_fp(fp_lt) || type_is_fp(fp_rt)))` at `src/codegen.c:4358` emits `cvtsi2sd`/`cvtsi2ss` for integer operands in binary FP expressions.
- **"Unary + on floating-point"** — Lines 3443–3449 of `src/codegen.c` handle unary `+` on FP as a no-op.

Stage 125 closes these stale items. The implementation work below covers the two genuine remaining gaps.

---

## Task 1 — FP File-Scope Globals Initialized from Integer Literals

### Root Cause

For `double x = 5;` at file scope, `codegen_add_global` (`src/codegen.c`, circa line 6832) processes the `AST_INT_LITERAL` initializer:

```c
} else if (init->type == AST_INT_LITERAL) {
    long v = strtol(init->value, NULL, 0);
    gv->init_value = v;        /* stores raw long */
    gv->is_initialized = 1;
}
```

In `codegen_emit_data`, for a `TYPE_DOUBLE` global with `init_value = 5`:
```c
fprintf(cg->output, "%s: %s %ld\n",
        gv->name, data_init_directive(gv->kind), gv->init_value);
```
`data_init_directive(TYPE_DOUBLE)` returns `"dq"`, so this emits:
```asm
x: dq 5
```
NASM interprets `dq 5` as the 64-bit **integer** 0x0000000000000005, not IEEE 754 double 5.0 (0x4014000000000000). Any code that uses `x` as a double will get garbage.

The same bug applies to `float f = 5;` — emits `dd 5` (integer bits) instead of `dd 5.0` (IEEE 754 float).

### Contrast with the Working Path

For `double x = 5.0;`, the initializer is `AST_FLOAT_LITERAL` and `codegen_add_global` stores:
```c
gv->init_label = init->value;    /* "5.0" — the raw source text */
gv->is_label_init = 0;
gv->is_initialized = 1;
```
`codegen_emit_data` sees `init_label != NULL && !is_label_init` and emits:
```asm
x: dq 5.0
```
NASM parses `5.0` as an FP constant and emits the correct IEEE 754 encoding. The fix for Task 1 uses this same path.

### Fix (`src/codegen.c` — `codegen_add_global`)

In the `AST_INT_LITERAL` branch, detect when the global's declared type is FP and convert the integer value to a floating-point string before storing in `init_label`:

```c
} else if (init->type == AST_INT_LITERAL) {
    long v = strtol(init->value, NULL, 0);
    if (gv->kind == TYPE_FLOAT || gv->kind == TYPE_DOUBLE) {
        /* Integer literal initializing a float/double global.
         * Emit via init_label so NASM encodes IEEE 754, not raw integer bits. */
        char buf[64];
        if (gv->kind == TYPE_FLOAT) {
            float fv = (float)v;
            snprintf(buf, sizeof(buf), "%.9g", (double)fv);
        } else {
            double dv = (double)v;
            snprintf(buf, sizeof(buf), "%.17g", dv);
        }
        /* Ensure NASM sees a decimal point (so it parses as FP, not integer). */
        if (!strchr(buf, '.') && !strchr(buf, 'e') && !strchr(buf, 'E'))
            strncat(buf, ".0", sizeof(buf) - strlen(buf) - 1);
        gv->init_label = codegen_intern(cg, buf);
        gv->is_label_init = 0;
        gv->is_initialized = 1;
    } else {
        gv->init_value = v;
        gv->is_initialized = 1;
    }
}
```

**Why `%.17g` for double?** — 17 significant digits is the round-trip minimum for a 64-bit double. `%.9g` for float is analogous (9 digits for 32-bit).

**Why check `strchr(buf, '.')`?** — `%.17g` for the value 5.0 produces `"5"` (no decimal point or exponent). Without the suffix, NASM would again treat it as integer. The suffix guarantees NASM parses it as an FP constant.

**Alternative encoding** (more explicit): emit the IEEE 754 bit pattern directly:
```c
union { double d; uint64_t u; } conv;
conv.d = (double)v;
snprintf(buf, sizeof(buf), "0x%016lx", conv.u);  /* hex bits for NASM */
```
NASM's `dq 0x4014000000000000` is unambiguous. Either approach works; the decimal string approach is more readable.

### Tests (`test/valid/floating_point/`)

- **`test_double_global_from_int__0.c`**:
  ```c
  double x = 5;
  int main(void) { return (x == 5.0) ? 0 : 1; }
  ```
  Expected exit: 0.

- **`test_float_global_from_int__0.c`**:
  ```c
  float f = 3;
  int main(void) { return (f == 3.0f) ? 0 : 1; }
  ```
  Expected exit: 0.

- **`test_double_global_from_zero__0.c`**:
  ```c
  double x = 0;
  int main(void) { return (x == 0.0) ? 0 : 1; }
  ```
  Expected exit: 0 (regression guard — `0` already worked via BSS/zero-init, but this confirms `init_value=0` path still handled).

- **`test_double_global_negative_from_int__0.c`**:
  ```c
  double x = -7;
  int main(void) { return (x == -7.0) ? 0 : 1; }
  ```
  Note: the unary `-` on the initializer is folded before reaching `codegen_add_global` as a negative integer literal, or it may be a `AST_UNARY_OP(-, AST_INT_LITERAL(7))`. Investigate at implementation time and handle the unary-minus-on-int-literal case by evaluating the expression to its negative value.

---

## Task 2 — Float-to-Double Promotion in Variadic Calls

### Specification

C99 §6.5.2.2p7 (default argument promotions):

> If the expression that denotes the called function has a type that does not include a prototype, the integer promotions are performed on each argument, and arguments that have type `float` are promoted to `double`.

C99 §6.5.2.2p7 (variadic arguments):

> The ellipsis notation in a function prototype declarator causes argument type conversion to stop after the last declared parameter. The default argument promotions are performed on trailing arguments.

So for any call to a variadic function (including `printf`, `fprintf`, `sprintf`, `scanf`), `float` arguments beyond the fixed parameters must be promoted to `double` before being stored in the XMM register or stack slot. A `float` argument passed as-is gives the callee 32 bits of float data in a 64-bit slot — `va_arg(ap, double)` would read garbage from the upper 32 bits.

### Root Cause

The caller-side argument emission loop (around `src/codegen.c:4800+`) stores the result type of each argument and selects `movss`/`movsd` accordingly. For a variadic `float` argument, it emits:
```asm
movss xmm0, [rbp - N]     ; load 32-bit float
```
The SysV AMD64 ABI requires this to be a `double` in the XMM register for variadic arguments. The caller must emit `cvtss2sd xmm0, xmm0` after loading.

### Diagnosis

When `printf("%f\n", 1.5f)` is compiled with our compiler:
1. `1.5f` is `TYPE_FLOAT`; it gets stored in xmm0 as a 32-bit float via `movss xmm0, dword [...]`
2. `al = 1` (one XMM register used)
3. `printf` calls `va_arg(ap, double)` to read the `%f` argument
4. `va_arg` reads 8 bytes from the XMM slot — gets `0x3FC0000000000000` treated as a double, which is approximately 2.9e-5, not 1.5

Expected: `cvtss2sd xmm0, xmm0` must follow the float load so the XMM register contains a proper double.

### Fix (`src/codegen.c` — caller arg emission)

In the call argument emission loop, after loading a `float` argument and before storing to the XMM arg slot, check if the call is variadic and if the argument is past the last fixed parameter:

```c
/* Variadic float → double promotion (C99 §6.5.2.2p7). */
if (is_variadic && arg_idx >= fixed_param_count &&
    arg_result_type->kind == TYPE_FLOAT) {
    fprintf(cg->output, "    cvtss2sd xmm%d, xmm%d\n",
            xmm_idx, xmm_idx);
}
```

The condition: `is_variadic && arg_idx >= fixed_param_count`.

- `is_variadic`: available from the call node's declaration or `FuncDecl.is_variadic`.
- `fixed_param_count`: the number of fixed (non-`...`) parameters.
- `arg_result_type`: the argument's `result_type` (or `full_type`), `TYPE_FLOAT`.

**Stack-slot case**: If the float argument overflows xmm0–xmm7 and is passed on the stack (the `is_fp && stack_passed` path from Stage 123), the promotion must happen before the `movss [rsp+N], xmm0` store:
```asm
; Before Stage 125:
movss xmm0, [rbp - M]
movss [rsp + N], xmm0      ; 32-bit float in stack slot — wrong

; After Stage 125:
movss xmm0, [rbp - M]
cvtss2sd xmm0, xmm0         ; promote to double
movsd [rsp + N], xmm0      ; 64-bit double in stack slot — correct
```
The stack store directive also changes from `movss` to `movsd` after promotion.

### Implementation Notes

Finding `fixed_param_count` at the call site:
- If the callee is a `AST_VAR_REF` resolving to a `FuncDecl`, read `decl->param_count`.
- If indirect call, the function pointer's type carries `is_variadic` and `param_count`.
- `arg_idx` is the loop counter (0-based) over actual arguments.

**When in doubt, conservative**: if `is_variadic` is true and the arg type is float, always promote. This is always correct: a fixed-parameter float arg is also promoted if the function is variadic (C99 §6.5.2.2p7 applies to all args in a K&R-style call, but for prototype variadic functions the fixed params have declared types and the compiler knows them — however, promoting is harmless because the fixed-param float is passed in xmm0 as a double, and the callee expects a double there. The only issue is if the callee has `float f` as a declared fixed parameter — but then the compiler would have stored `TYPE_FLOAT` as the parameter type and would do `cvtsd2ss` when reading it. In practice, promote all float args in all variadic calls and the result is always correct).

### Tests (`test/valid/varargs/`)

- **`test_variadic_float_printf.c`** + `.expected`:
  ```c
  #include <stdio.h>
  int main(void) {
      float f = 1.5f;
      printf("%.1f\n", f);
      return 0;
  }
  ```
  `.expected`:
  ```
  1.5
  ```

- **`test_variadic_float_direct.c`** + `.expected`:
  ```c
  #include <stdio.h>
  #include <stdarg.h>
  static double sum2(int n, ...) {
      va_list ap;
      va_start(ap, n);
      double a = va_arg(ap, double);
      double b = va_arg(ap, double);
      va_end(ap);
      return a + b;
  }
  int main(void) {
      float x = 1.5f, y = 2.5f;
      printf("%.1f\n", sum2(2, x, y));
      return 0;
  }
  ```
  `.expected`:
  ```
  4.0
  ```

- **`test_variadic_int_unaffected__5.c`**: Regression guard — passing integer args to variadic function still works.
  ```c
  #include <stdarg.h>
  static int sum(int n, ...) {
      va_list ap; va_start(ap, n); int r = 0;
      while (n--) r += va_arg(ap, int);
      va_end(ap); return r;
  }
  int main(void) { return sum(3, 1, 2, 3) - 1; }
  ```
  Expected exit: 5.

---

## Task 3 — Stale Checklist Cleanup

### Background

The checklist at `docs/outlines/checklist.md` has two items marked incomplete that were implemented in Stage 110:

- `[ ] Mixed integer/floating-point arithmetic` — DONE in Stage 110 (binary op path `src/codegen.c:4358`)
- `[ ] Unary + on floating-point` — DONE in Stage 110 (`src/codegen.c:3443`)

### Fix (`docs/outlines/checklist.md`)

Change both items from `[ ]` to `[x]` and annotate the stage:

```markdown
- [x] Mixed integer/floating-point arithmetic (Stage 110)
- [x] Unary + on floating-point (Stage 110)
```

No code changes. Verify no other stale items are present by cross-referencing the checklist against the stage list.

---

## Version

Bump `VERSION_STAGE` in `src/version.c` to `"01250000"`.

---

## Implementation Order

1. Task 1 (FP global from int init) — add the FP check in `codegen_add_global`, run the new tests.
2. Task 2 (variadic float promotion) — locate the variadic arg emit path, add `cvtss2sd`, run the variadic tests.
3. Task 3 (checklist) — update `docs/outlines/checklist.md`.
4. Full test suite: `./test/run_all_tests.sh`.
5. Bump `src/version.c`.
6. Self-host: `./build.sh --mode=self-host`.
7. Commit.

---

## Out of Scope

- **FP constant expressions as file-scope initializers**: `double x = 1.0 + 2.0;` — constant-folding FP arithmetic at compile time requires either a constant-expression evaluator for FP nodes or accepting arbitrary initializers via static constructors. Neither is in scope for this compiler.
- **`long double`**: Not supported; not tracked.
- **Default argument promotions for K&R function calls** (no prototype): This compiler requires prototypes for all calls; K&R style is not supported.
- **FP compound literals at file scope** (`double *p = &(double){3.14};`): Covered by the Task 3 restriction note in Stage 124 spec.
