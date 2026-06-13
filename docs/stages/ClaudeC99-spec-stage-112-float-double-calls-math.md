# ClaudeC99 Stage 112 — `float`/`double` Calling Convention, `va_arg`, and `<math.h>`

## Goal

Complete floating-point support by implementing the SysV AMD64 calling
convention for FP arguments and return values, extending `va_arg` for
`double`, and providing a `<math.h>` stub:

```c
#include <math.h>
#include <stdio.h>

double square(double x) { return x * x; }

int main(void) {
    double r = sqrt(2.0);
    printf("sqrt(2) = %.6f\n", r);        /* variadic FP arg */
    printf("square(3) = %.0f\n", square(3.0));
    return 0;
}
```

Prerequisites: Stages 109–111 must be complete.

---

## Background — SysV AMD64 ABI for FP

The SysV AMD64 ABI (System V Application Binary Interface — AMD64
Architecture Processor Supplement §3.2.3) classifies function arguments
into INTEGER class (passed in `rdi`/`rsi`/`rdx`/`rcx`/`r8`/`r9`) and
SSE class (passed in `xmm0`–`xmm7`). Integer and SSE slots are counted
independently: the first SSE-class argument always goes in `xmm0`
regardless of how many INTEGER arguments came before it.

Return values: an SSE-class return value goes in `xmm0`.

**Variadic functions**: before `call` to a variadic function, the `al`
register must contain the number of SSE (XMM) registers used for
arguments (0–8). The callee's prologue uses `al` to decide how many
`xmm` registers to save.

**`va_list` register-save area layout**:
```
[reg_save_area + 0]   rdi  (8 bytes, GP reg 0)
[reg_save_area + 8]   rsi  (8 bytes, GP reg 1)
...
[reg_save_area + 40]  r9   (8 bytes, GP reg 5)
[reg_save_area + 48]  xmm0 (16 bytes, XMM reg 0)
[reg_save_area + 64]  xmm1 (16 bytes, XMM reg 1)
...
[reg_save_area + 160] xmm7 (16 bytes, XMM reg 7)
```

Total: 48 bytes GP + 128 bytes XMM = 176 bytes.

`fp_offset` starts at 48. Each `va_arg(ap, double)` loads from
`reg_save_area + fp_offset` (a 16-byte XMM save slot; only the low 8
bytes are the double) and advances `fp_offset += 16`. When
`fp_offset >= 176`, the argument is on the overflow stack.

---

## C99 references

| Feature | C99 section |
|---------|-------------|
| Argument promotion in variadic calls | §6.5.2.2p6 |
| `va_arg` | §7.15.1.1 |
| `<math.h>` | §7.12 |

---

## Task 1 — Non-variadic call sites with FP arguments (`src/codegen.c`)

### 1a — Argument classification loop

The call-site argument loop currently increments `int_arg_count` for each
argument and places integer values in `rdi`/`rsi`/… slots. Extend it with
a parallel `xmm_arg_count` counter:

- When the argument type is `TYPE_FLOAT` or `TYPE_DOUBLE`: place in
  `xmm{xmm_arg_count}` and increment `xmm_arg_count`.
- When the argument type is integer/pointer: place in the next INTEGER
  register slot (as before).
- If `xmm_arg_count > 7`: pass the argument on the stack (same overflow
  path as integer arguments beyond 6).

**Argument staging**: the evaluation of each argument leaves its value in
`xmm0` (for FP) or `rax` (for integer). Staging to the target XMM
register:

```nasm
; Stage double into xmm1 (second FP arg):
movsd xmm1, xmm0    ; or movss for float
```

For calls with a mix of integer and FP arguments, the integer args must
be in `rdi`/`rsi`/… and the FP args in `xmm0`/`xmm1`/… simultaneously
when `call` is executed. The safest approach: evaluate all arguments, push
each to the stack in reverse, then load the final register slots in order.

> The current codegen already stages integer args by evaluating into `rax`
> and moving to the target register. FP args follow the same pattern using
> `movsd xmmN, xmm0`.

### 1b — Return value in `xmm0`

When the callee's declared return type is `TYPE_FLOAT` or `TYPE_DOUBLE`,
the return value arrives in `xmm0` after the `call`. No additional move
is needed (it is already in the correct FP result register). Callers
that need to store the return value emit a `movsd [rbp - N], xmm0` as
for any other FP store.

---

## Task 2 — Non-variadic function prologues with FP parameters (`src/codegen.c`)

For each parameter with `TYPE_FLOAT` or `TYPE_DOUBLE`, the prologue must
move the incoming value from `xmmN` to the local stack slot:

```nasm
; double-typed first parameter → xmm0 → [rbp - 8]:
movsd [rbp - 8], xmm0

; float-typed second FP parameter → xmm1 → [rbp - 12]:
movss [rbp - 12], xmm1
```

The `xmm_param_index` counter tracks how many FP parameters have been
seen so far (analogous to the existing `int_param_index`).

---

## Task 3 — Return value from FP functions (`src/codegen.c`)

When `return <expr>` appears inside a function with a float/double return
type:
1. Evaluate `<expr>` into `xmm0`.
2. If needed, convert (e.g. `int` → `double` via `cvtsi2sd xmm0, rax`).
3. Emit the function epilogue (`pop rbp; ret`) as usual.

The calling convention leaves the return value in `xmm0` — no extra move
is needed.

---

## Task 4 — Variadic calls with FP arguments (`src/codegen.c`)

The `al` register must be set to the number of XMM registers used before
calling any variadic function:

```nasm
mov al, 1          ; one XMM register used (e.g. printf("%f", d))
call printf
```

**Change**: extend the call-site codegen to always emit `mov al,
<xmm_arg_count>` immediately before the `call` instruction when calling
a variadic function. For non-variadic functions, `al` is caller-saved and
need not be set (though setting it is harmless).

**Identifying variadic callees**: the compiler already tracks `is_variadic`
on function declarations in the `funcs` table (set when `...` appears in
the parameter list). Use this flag at the call site.

---

## Task 5 — Variadic function prologues: save XMM registers (`src/codegen.c`)

When a variadic function definition is compiled, its prologue must save
`xmm0`–`xmm7` into the register-save area. Currently only the GP registers
are saved.

**Current prologue** (for a variadic function):
```nasm
; Save GP registers (rdi..r9) to reg_save_area:
mov [rbp - <rsave> + 0],  rdi
mov [rbp - <rsave> + 8],  rsi
...
mov [rbp - <rsave> + 40], r9
```

**Extension**: immediately after the GP saves, save the 8 XMM registers
(16 bytes each, using `movaps` which requires 16-byte alignment):

```nasm
movaps [rbp - <rsave> + 48],  xmm0
movaps [rbp - <rsave> + 64],  xmm1
movaps [rbp - <rsave> + 80],  xmm2
movaps [rbp - <rsave> + 96],  xmm3
movaps [rbp - <rsave> + 112], xmm4
movaps [rbp - <rsave> + 128], xmm5
movaps [rbp - <rsave> + 144], xmm6
movaps [rbp - <rsave> + 160], xmm7
```

The `reg_save_area` allocation in the stack frame must grow from 48 bytes
(GP only) to 176 bytes (48 GP + 128 XMM). This is a change to how
`va_start` reserves space.

**`movaps` alignment requirement**: `[rbp - <rsave> + 48]` must be
16-byte aligned. Ensure the `reg_save_area` base is 16-byte aligned in
the frame layout.

> **Optimisation note**: the ABI allows the callee to save only the first
> `al` XMM registers. ClaudeC99 saves all 8 unconditionally for
> simplicity — this is correct but slightly over-conservative.

---

## Task 6 — `va_arg` for `double` (`src/codegen.c`)

C99 §6.5.2.2p6: `float` arguments in variadic calls are always promoted
to `double`. Therefore `va_arg(ap, float)` is undefined; `va_arg(ap,
double)` is the only supported FP case.

**Current `va_arg` codegen** handles GP types by reading from
`reg_save_area + gp_offset` (if `gp_offset < 48`) or `overflow_arg_area`.

**Extension** for `va_arg(ap, double)`:

```nasm
; ap is at [rbp - ap_off]
mov  rcx, [rbp - ap_off + 16]   ; reg_save_area pointer (offset 16 in va_list)
mov  eax, [rbp - ap_off + 4]    ; fp_offset (bytes 4–7 in va_list struct)
cmp  eax, 176
jge  .overflow_N

; Register path:
add  rax, rcx                   ; reg_save_area + fp_offset
movsd xmm0, [rax]               ; load the double (low 8 bytes of 16-byte slot)
add  dword [rbp - ap_off + 4], 16  ; fp_offset += 16
jmp  .done_N

.overflow_N:
mov  rax, [rbp - ap_off + 8]    ; overflow_arg_area pointer
movsd xmm0, [rax]               ; load double from overflow area
add  qword [rbp - ap_off + 8], 8   ; advance overflow pointer by 8

.done_N:
; result in xmm0
```

> The result lands in `xmm0` (consistent with the FP result register
> convention from Stage 109).

---

## Task 7 — `test/include/math.h` stub

Create `test/include/math.h`:

```c
#ifndef CLAUDEC99_MATH_H
#define CLAUDEC99_MATH_H

/* Basic math functions — double precision */
double sin(double x);
double cos(double x);
double tan(double x);
double asin(double x);
double acos(double x);
double atan(double x);
double atan2(double y, double x);
double sinh(double x);
double cosh(double x);
double tanh(double x);
double exp(double x);
double log(double x);
double log2(double x);
double log10(double x);
double pow(double x, double y);
double sqrt(double x);
double cbrt(double x);
double fabs(double x);
double floor(double x);
double ceil(double x);
double round(double x);
double trunc(double x);
double fmod(double x, double y);
double fmin(double x, double y);
double fmax(double x, double y);
double hypot(double x, double y);

/* Single-precision variants */
float sinf(float x);
float cosf(float x);
float tanf(float x);
float expf(float x);
float logf(float x);
float log2f(float x);
float log10f(float x);
float powf(float x, float y);
float sqrtf(float x);
float fabsf(float x);
float floorf(float x);
float ceilf(float x);
float roundf(float x);
float truncf(float x);
float fmodf(float x, float y);
float fminf(float x, float y);
float fmaxf(float x, float y);

/* Classification macros (integer-returning) */
int isnan(double x);
int isinf(double x);
int isfinite(double x);
int isnormal(double x);

/* Constants */
#define M_PI     3.14159265358979323846
#define M_E      2.71828182845904523536
#define M_SQRT2  1.41421356237309504880
#define M_LN2    0.69314718055994530942
#define M_LOG2E  1.44269504088896340736
#define HUGE_VAL __builtin_huge_val()
#define INFINITY __builtin_inff()
#define NAN      __builtin_nanf("")

#endif
```

> `HUGE_VAL`, `INFINITY`, and `NAN` use GCC/Clang builtins; if those
> fail under ClaudeC99, remove them and note the limitation.
> `isnan`/`isinf`/`isfinite` are declared as functions for simplicity
> (the standard defines them as macros, but function declarations work
> for direct calls).

---

## Task 8 — Version bump (`src/version.c`)

```c
#define VERSION_STAGE  "01120000"
```

---

## Implementation order

1. `src/codegen.c` — non-variadic FP parameter prologue (Task 2)
2. `src/codegen.c` — non-variadic FP call-site argument staging (Task 1)
3. `src/codegen.c` — FP return values (Task 3)
4. `src/codegen.c` — variadic call `al` register (Task 4)
5. `src/codegen.c` — variadic prologue XMM register save (Task 5)
6. `src/codegen.c` — `va_arg` for `double` (Task 6)
7. `test/include/math.h` — stub header (Task 7)
8. `src/version.c` — version bump
9. Tests
10. Build, full test suite, self-host cycle

---

## Out of scope — do NOT do these in this stage

- **`va_arg(ap, float)`** — undefined by C99 (float is always promoted
  to double in variadic calls); emit a compile error if used.
- **`long double`** — deferred indefinitely.
- **`complex.h` / `fenv.h`** — deferred.
- **`isnan`/`isinf`/`isfinite` as macro expressions** — declared as
  functions in the stub; full macro forms deferred.
- **`HUGE_VAL`, `INFINITY`, `NAN` constants** — stub declarations
  only; if `__builtin_huge_val()` etc. do not compile under ClaudeC99,
  remove them from the stub.
- **Struct-returning math functions** (`div_t` return values) — already
  in `<stdlib.h>`; no change needed here.

---

## Tests

### `test_fp_func_call__0.c` — function taking and returning double

```c
static double square(double x) { return x * x; }
int main(void) {
    double r = square(7.0);
    return (int)r - 49;
}
```

Expected exit: 0.

### `test_fp_mixed_params__0.c` — function with mixed int/double parameters

```c
static double scale(int n, double factor) { return n * factor; }
int main(void) {
    double r = scale(6, 7.0);
    return (int)r - 42;
}
```

Expected exit: 0.

### `test_fp_return_float__0.c` — function returning float

```c
static float half(float x) { return x / 2.0f; }
int main(void) {
    float r = half(84.0f);
    return (int)r - 42;
}
```

Expected exit: 0.

### `test_fp_sqrt__0.c` — call `sqrt` from `<math.h>`

```c
#include <math.h>
int main(void) {
    double r = sqrt(9.0);
    return (int)r - 3;
}
```

Expected exit: 0.

### `test_fp_printf__0.c` — `printf` with double argument (variadic FP)

```c
#include <stdio.h>
int main(void) {
    double d = 42.0;
    printf("%.0f\n", d);
    return 0;
}
```

Expected exit: 0.  Expected output (checked via integration test if
stdout capture is available): `42`.

### `test_fp_varargs__0.c` — `va_arg` for double

```c
#include <stdarg.h>
static double sum(int n, ...) {
    va_list ap;
    va_start(ap, n);
    double s = 0.0;
    int i;
    for (i = 0; i < n; i++)
        s = s + va_arg(ap, double);
    va_end(ap);
    return s;
}
int main(void) {
    double r = sum(3, 1.0, 2.0, 3.0);
    return (int)r - 6;
}
```

Expected exit: 0.

### `test_fp_pow__0.c` — call `pow` from `<math.h>`

```c
#include <math.h>
int main(void) {
    double r = pow(2.0, 10.0);
    return (int)r - 1024;
}
```

Expected exit: 0.

---

## Docs (at stage close, after all tests pass)

- **`README.md`** — update "Through stage …" line; note FP calling
  convention, `va_arg` for double, and `<math.h>` are supported; remove
  `float`/`double` from the "Not yet supported" section; refresh test
  totals.
- **`docs/outlines/checklist.md`** — add Stage 112 section; tick
  `float`/`double` in TODO.
- Run the **`update-supplemental-docs`** skill.
- **`docs/self-compilation-report.md`** — record Stage 112 self-host run.
