# Stage 125 Kickoff — FP Globals from Integer Initializers, Variadic Float Promotion, and Stale Checklist Cleanup

## Overview

Stage 125 addresses two floating-point correctness bugs and closes out stale documentation items:

1. **FP file-scope globals initialized from integer literals** — `double x = 5;` currently emits integer bits instead of IEEE 754
2. **Float-to-double promotion in variadic function calls** — C99 §6.5.2.2p7 requires `float` args to be promoted to `double`
3. **Stale checklist cleanup** — mark two items from Stage 110 as complete

These tasks are independent and can be implemented in sequence.

## Spec Summary

### Task 1 — FP File-Scope Globals from Integer Initializers

**Root Cause:** For `double x = 5;` at file scope, the parser accepts `AST_INT_LITERAL` but `codegen_add_global` stores the raw integer value (5) and emits `dq 5`, which NASM interprets as the 64-bit integer 0x0000000000000005, not IEEE 754 double 5.0 (0x4014000000000000). The same bug affects `float f = 3;` (emits `dd 3` instead of `dd 3.0`).

**Working Path:** For `double x = 5.0;` (FP literal), `codegen_add_global` stores the source text "5.0" in `init_label` and emits `dq 5.0`, which NASM correctly parses as an FP constant.

**Fix:** In `codegen_add_global`, when processing `AST_INT_LITERAL` for a `TYPE_FLOAT` or `TYPE_DOUBLE` global, convert the integer value to a floating-point string using `snprintf` with format `%.9g` (float) or `%.17g` (double), ensure the string contains a decimal point or exponent to force NASM to parse as FP, and store in `init_label`.

**Negative integers:** For `double x = -7;`, the parser should fold `AST_UNARY_OP("-", AST_INT_LITERAL("7"))` into a negative integer literal before reaching codegen, or accept the unary-op and evaluate it to the negative value.

### Task 2 — Float-to-Double Promotion in Variadic Calls

**Specification:** C99 §6.5.2.2p7 — default argument promotions apply to variadic arguments beyond the fixed parameters. This includes promoting `float` to `double`.

**Root Cause:** For `printf("%.1f\n", 1.5f)`, the caller emits `movss xmm0, dword [...]` to load the 32-bit float into xmm0. The callee reads this via `va_arg(ap, double)`, which expects 8 bytes of IEEE 754 double in the XMM slot — but only 32 bits of valid data are present. Result: garbage output.

**Fix:** After loading a `float` argument into an XMM register for a variadic function call, if the argument index is beyond the fixed parameter count, emit `cvtss2sd xmm<N>, xmm<N>` to promote the float to double. For stack-passed float arguments (overflow from xmm0–xmm7), emit the promotion before storing to the stack slot and change the store directive from `movss` to `movsd`.

### Task 3 — Stale Checklist Cleanup

Two items in `docs/outlines/checklist.md` are marked incomplete but were implemented in Stage 110:
- "Mixed integer/floating-point arithmetic" — codegen path at `src/codegen.c:4358` handles `cvtsi2sd`/`cvtsi2ss`
- "Unary + on floating-point" — codegen path at `src/codegen.c:3443` handles unary `+` on FP as no-op

**Fix:** Mark both items as complete with stage annotation.

---

## Required Changes

### 1. Tokenizer Changes (`src/lexer.c`)

**No changes required.** Integer and floating-point literal parsing is unchanged.

### 2. Parser Changes (`src/parser.c`)

**File:** `src/parser.c`, lines ~3798–3806 (global initializer validation)

Extend the float/double global initializer check to accept `AST_INT_LITERAL`:

```c
// Before: only AST_FLOAT_LITERAL allowed
if (init->type != AST_FLOAT_LITERAL) {
    PARSER_ERROR(...);
}

// After: allow AST_INT_LITERAL and folded unary minus
if (init->type != AST_FLOAT_LITERAL && init->type != AST_INT_LITERAL &&
    init->type != AST_UNARY_OP) {
    PARSER_ERROR(...);
}
```

**Optional optimization:** If the parser encounters `AST_UNARY_OP("-", AST_INT_LITERAL("7"))` during parsing of the initializer, fold it into `AST_INT_LITERAL("-7")` before storing in the global declaration. This simplifies codegen. Alternatively, accept the unary-op node and have codegen evaluate it.

### 3. AST Changes

**No changes required.** Existing AST node types suffice.

### 4. Code Generation Changes (`src/codegen.c`)

#### 4a. Task 1 — FP Global from Integer Init

**In `codegen_add_global` (around line 6832):**

Modify the `AST_INT_LITERAL` case to detect FP globals:

```c
} else if (init->type == AST_INT_LITERAL) {
    long v = strtol(init->value, NULL, 0);
    if (gv->kind == TYPE_FLOAT || gv->kind == TYPE_DOUBLE) {
        /* Integer literal initializing a float/double global.
         * Convert to FP string for NASM IEEE 754 encoding. */
        char buf[64];
        if (gv->kind == TYPE_FLOAT) {
            float fv = (float)v;
            snprintf(buf, sizeof(buf), "%.9g", (double)fv);
        } else {
            double dv = (double)v;
            snprintf(buf, sizeof(buf), "%.17g", dv);
        }
        /* Ensure decimal point or exponent so NASM parses as FP, not integer. */
        if (!strchr(buf, '.') && !strchr(buf, 'e') && !strchr(buf, 'E'))
            strncat(buf, ".0", sizeof(buf) - strlen(buf) - 1);
        gv->init_label = codegen_intern(cg, buf);
        gv->is_label_init = 0;
        gv->is_initialized = 1;
    } else {
        /* Integer or pointer global. */
        gv->init_value = v;
        gv->is_initialized = 1;
    }
}
```

**Why `%.17g` for double?** — 17 significant digits is the round-trip minimum for IEEE 754 double. `%.9g` for float is analogous (9 digits for 32-bit single).

**Why check for `.` / `e` / `E`?** — `%.17g` for the value 5.0 produces "5" (no decimal point). Without the ".0" suffix, NASM would parse it as integer. The suffix forces FP interpretation.

**Handling unary minus:** If the parser passes `AST_UNARY_OP("-", AST_INT_LITERAL("7"))`, codegen must evaluate it to the negative value:

```c
} else if (init->type == AST_UNARY_OP && init->op[0] == '-') {
    if (init->child_count > 0 && init->children[0]->type == AST_INT_LITERAL) {
        long v = -strtol(init->children[0]->value, NULL, 0);
        // [same FP conversion logic as above with the negative v]
    }
}
```

#### 4b. Task 2 — Variadic Float Promotion

**In the call argument emission loop (around `src/codegen.c:4800+`):**

After evaluating a `float` argument and before storing to the XMM arg slot, check if the call is variadic and if the argument is beyond the fixed parameter count:

```c
/* Variadic float → double promotion (C99 §6.5.2.2p7). */
if (is_variadic && arg_idx >= fixed_param_count &&
    arg_result_type->kind == TYPE_FLOAT) {
    fprintf(cg->output, "    cvtss2sd xmm%d, xmm%d\n",
            xmm_idx, xmm_idx);
}
```

**Finding `is_variadic` and `fixed_param_count`:**
- If the callee is `AST_VAR_REF` resolving to a `FuncDecl`, read `decl->is_variadic` and `decl->param_count`.
- If the call is indirect (function pointer), the pointer's `Type` carries `is_variadic` and `param_count`.

**Stack-slot case:** If the float argument overflows xmm0–xmm7 and is passed on the stack (the `stack_passed` path from Stage 123):

```asm
; Before Stage 125:
movss xmm0, [rbp - M]
movss [rsp + N], xmm0      ; 32-bit float in stack slot — wrong

; After Stage 125:
movss xmm0, [rbp - M]
cvtss2sd xmm0, xmm0         ; promote to double
movsd [rsp + N], xmm0      ; 64-bit double in stack slot — correct
```

Change the stack store directive from `movss` to `movsd` after promotion.

**Conservative approach:** If `is_variadic` is true and the arg type is `TYPE_FLOAT`, always promote. This is always correct: promoting a fixed-parameter float in a variadic call is harmless because the callee expects a double there.

### 5. Checklist Update (`docs/outlines/checklist.md`)

**File:** `docs/outlines/checklist.md`

Find and update:
```markdown
- [ ] Mixed integer/floating-point arithmetic
- [ ] Unary + on floating-point
```

Change to:
```markdown
- [x] Mixed integer/floating-point arithmetic (Stage 110)
- [x] Unary + on floating-point (Stage 110)
```

### 6. Version Bump (`src/version.c`)

**File:** `src/version.c`

Change `VERSION_STAGE` from `"01240000"` to:
```c
#define VERSION_STAGE "01250000"
```

---

## Test Plan

### Task 1 Tests — FP Globals from Integer Initializers

**Valid tests** (in `test/valid/floating_point/`):

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
  Expected exit: 0 (regression guard — zero-init path unchanged).

- **`test_double_global_negative_from_int__0.c`**:
  ```c
  double x = -7;
  int main(void) { return (x == -7.0) ? 0 : 1; }
  ```
  Expected exit: 0.

### Task 2 Tests — Variadic Float Promotion

**Valid tests** (in `test/valid/varargs/`):

- **`test_variadic_float_printf.c`** + `.expected`:
  ```c
  #include <stdio.h>
  int main(void) {
      float f = 1.5f;
      printf("%.1f\n", f);
      return 0;
  }
  ```
  `.expected`: `1.5`

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
  `.expected`: `4.0`

- **`test_variadic_int_unaffected__5.c`** (regression guard):
  ```c
  #include <stdarg.h>
  static int sum(int n, ...) {
      va_list ap; va_start(ap, n); int r = 0;
      while (n--) r += va_arg(ap, int);
      va_end(ap); return r;
  }
  int main(void) { return sum(3, 1, 2, 3) - 5; }
  ```
  Expected exit: 0 (1 + 2 + 3 - 5).

---

## Implementation Order

1. Parser fix (accept `AST_INT_LITERAL` for FP globals)
2. Codegen Task 1 (convert integer to FP string in `codegen_add_global`)
3. Codegen Task 2 (emit `cvtss2sd` for variadic float args)
4. Checklist update
5. Test creation
6. Version bump
7. Full test suite: `./test/run_all_tests.sh`
8. Self-host: `./build.sh --mode=self-host`
9. Commit with Co-Authored-By trailer

---

## Notes & Decisions

- **FP string encoding:** `%.17g` for double, `%.9g` for float ensures round-trip correctness without excessive precision.
- **Decimal-point suffix:** Always append ".0" if the formatted string lacks a decimal or exponent, so NASM doesn't misinterpret as integer.
- **Unary minus folding:** The parser may fold `-(integer_literal)` into a single `AST_INT_LITERAL` with a negative value, or may pass the `AST_UNARY_OP` node to codegen. Implement whichever path the parser takes; codegen must handle both cases.
- **Variadic detection:** Use `is_variadic && arg_idx >= fixed_param_count` to safely promote only variadic args. Conservative: promoting all float args in a variadic call is always correct.
- **No AST changes:** Existing node types handle all required cases.
