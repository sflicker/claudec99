# ClaudeC99 Stage 110 — `float`/`double` Arithmetic, Conversions, and Casts

## Goal

Implement full arithmetic on `float` and `double` values and all the
type-conversion rules that govern mixed-type expressions:

```c
double a = 1.5 + 2.5;           /* 4.0 */
float  b = 3.0f * 2.0f;         /* 6.0f */
double c = b + 1;                /* int 1 widened to double */
int    n = (int)(c / 2.0);       /* explicit cast: 3 */
float  f = -(float)n;            /* unary minus */
```

Prerequisite: Stage 109 (types, literals, load/store) must be complete.

---

## Background — current state after Stage 109

After Stage 109 the compiler can declare float/double variables and load/store
them via `movss`/`movsd`. FP arithmetic expressions like `x + y` still
cause a code path that either errors ("not yet implemented") or incorrectly
falls through to integer arithmetic — both are bugs. Stage 110 fixes this
by wiring the SSE2 arithmetic instructions into the binary-operator codegen.

---

## C99 references

| Feature | C99 section |
|---------|-------------|
| Usual Arithmetic Conversions (real types) | §6.3.1.8 |
| Implicit conversion between real types | §6.3.1.5 |
| Floating-point arithmetic operators | §6.5.5, §6.5.6 |
| Cast expression | §6.5.4 |

---

## Task 1 — FP binary arithmetic (`src/codegen.c`)

### 1a — Instruction selection

The `codegen_binary_op` function (or equivalent) must select instructions
based on the operand type:

| Operator | `float` | `double` |
|----------|---------|----------|
| `+` | `addss xmm0, xmm1` | `addsd xmm0, xmm1` |
| `-` | `subss xmm0, xmm1` | `subsd xmm0, xmm1` |
| `*` | `mulss xmm0, xmm1` | `mulsd xmm0, xmm1` |
| `/` | `divss xmm0, xmm1` | `divsd xmm0, xmm1` |

Left operand in `xmm0`, right operand in `xmm1`, result in `xmm0`.
The result type of `float op float` is `float`; `double op double` is
`double`; mixed cases follow UAC (see Task 2).

### 1b — Saving the left operand during right-operand evaluation

The current integer binary-op codegen uses `push rax` to preserve the
left operand while evaluating the right. SSE2 has no `push xmm0`
instruction. Use explicit memory save/restore:

```nasm
; Save xmm0 (left operand) before evaluating right:
sub rsp, 8
movsd [rsp], xmm0        ; or movss for float (4 bytes, 4-byte offset is fine)

; ... evaluate right operand into xmm0 ...

; Restore into xmm1, pop:
movsd xmm1, [rsp]
add rsp, 8
; Now: xmm0 = right, xmm1 = left — swap if needed OR define convention:
; Convention: after pop, xmm1 = left, xmm0 = right; then emit op (xmm1 op= xmm0)
; OR: swap xmm0/xmm1 with movaps after restoring.
```

**Recommended convention**: after restoring, `xmm0` holds the right
operand and `xmm1` holds the left. Subtraction and division are not
commutative so the instruction must be `subsd xmm1, xmm0` (left minus
right) with the result in `xmm1`, then `movsd xmm0, xmm1`. For
commutative ops (`addsd`, `mulsd`) order does not matter.

Alternatively, swap the stack save/restore convention to match the
integer pattern exactly (`xmm1 = left-hand side after pop`).

> Whichever convention is chosen, document it in a comment and apply it
> consistently across all binary-op emission sites.

---

## Task 2 — Usual Arithmetic Conversions for FP (`src/codegen.c`, `src/parser.c`)

C99 §6.3.1.8: when operands have different real types, the narrower type
is converted to the wider before the operation:

1. If either operand is `double`, the other is converted to `double`.
2. Else if either operand is `float`, the other is converted to `float`.
3. (Stage 112 handles `long double` — currently deferred.)

**Parser-side**: in the binary-operator type resolution (wherever
`result_type` is assigned for `AST_BINARY_OP` nodes), extend to return
`type_double()` when either child is `double`, else `type_float()` when
either child is `float`, before the existing integer promotion rules.

**Codegen-side**: before emitting the binary instruction, check whether
a widening conversion is needed on the left or right operand and emit it:

- `float` → `double`: `cvtss2sd xmm0, xmm0` (or `xmm1` for the other
  operand after restore).
- `int` (in `rax`) → `float`: `cvtsi2ss xmm0, rax`.
- `int` (in `rax`) → `double`: `cvtsi2sd xmm0, rax`.

---

## Task 3 — Unary minus on FP (`src/codegen.c`)

Unary minus on a float/double toggles the sign bit. SSE2 has no
`negatess`/`negatesd` instruction; use XOR with a sign-bit mask constant:

```c
/* In .rodata (emit once per TU): */
Lfp_sign_mask_f32: DD 0x80000000
Lfp_sign_mask_f64: DQ 0x8000000000000000
```

```nasm
; Unary minus on xmm0 (double):
xorpd xmm0, [rel Lfp_sign_mask_f64]

; Unary minus on xmm0 (float):
xorps xmm0, [rel Lfp_sign_mask_f32]
```

The sign-mask constants are emitted on demand (once per translation unit,
when the first FP unary-minus is encountered). Use the same `.rodata`
literal table introduced in Stage 109.

---

## Task 4 — Explicit casts (`src/codegen.c`)

Extend the `AST_CAST` codegen to handle FP ↔ integer and FP ↔ FP
conversions:

| From | To | Instruction |
|------|-----|-------------|
| `int`/`long` (in `rax`) | `float`  | `cvtsi2ss xmm0, rax` |
| `int`/`long` (in `rax`) | `double` | `cvtsi2sd xmm0, rax` |
| `float` (in `xmm0`) | `double` | `cvtss2sd xmm0, xmm0` |
| `double` (in `xmm0`) | `float`  | `cvtsd2ss xmm0, xmm0` |
| `float` (in `xmm0`) | `int`/`long` | `cvttss2si rax, xmm0` |
| `double` (in `xmm0`) | `int`/`long` | `cvttsd2si rax, xmm0` |

**`cvttss2si` / `cvttsd2si`**: truncate toward zero (C99 §6.3.1.4
"When a finite value of real floating type is converted to an integer
type … the fractional part is discarded (i.e., the value is truncated
toward zero)").

**Unsigned integer targets**: C99 §6.3.1.4 leaves the behaviour
implementation-defined when the result cannot be represented. Use
`cvttsd2si rax, xmm0` unconditionally for this stage (correct for
non-negative values in range; UB for others).

**Parser-side**: `parse_cast` already produces `AST_CAST` nodes for
`(type-name) expr`; confirm that `TOKEN_FLOAT` and `TOKEN_DOUBLE` are
included in the `parse_type_name` lookahead sets (this was added in
Stage 109 Task 3a).

---

## Task 5 — Implicit int → FP conversion in assignments and initializers

C99 §6.3.1.5: an integer value assigned to a float/double variable is
implicitly converted. In codegen, when the RHS type is an integer type
and the LHS type is float/double, emit:

```nasm
cvtsi2ss xmm0, rax   ; int → float
cvtsi2sd xmm0, rax   ; int → double
```

before the store instruction. This applies in:
- `AST_ASSIGNMENT` (when types differ across the assignment).
- `AST_DECLARATION` with an initializer.
- Function return (return statement with int value in a float-returning
  function — Stage 112, but the conversion codegen is the same).

---

## Task 6 — Version bump (`src/version.c`)

```c
#define VERSION_STAGE  "01100000"
```

---

## Implementation order

1. `src/parser.c` — UAC type resolution for binary ops with FP operands
2. `src/codegen.c` — sign-mask constants in `.rodata`
3. `src/codegen.c` — FP binary arithmetic (Task 1)
4. `src/codegen.c` — unary minus for FP (Task 3)
5. `src/codegen.c` — explicit casts (Task 4)
6. `src/codegen.c` — implicit int → FP in assignments (Task 5)
7. `src/version.c` — version bump
8. Tests
9. Build, full test suite, self-host cycle

---

## Out of scope — do NOT do these in this stage

- **FP comparisons** (`<`, `==`, etc.) — Stage 111.
- **FP in `if`/`while` conditions** — Stage 111.
- **FP function parameters and return values** — Stage 112.
- **`%` (remainder) on FP** — undefined by C99; skip.
- **`long double`** — deferred indefinitely.
- **`NaN` / `Inf` literal syntax** — not in C99; skip.
- **Floating-point exceptions or `fenv.h`** — deferred.

---

## Tests

### `test_float_add__0.c` — basic float addition

```c
int main(void) {
    float a = 1.5f;
    float b = 2.5f;
    float c = a + b;
    return (int)c - 4;   /* 4.0 → int 4; exit 0 */
}
```

Expected exit: 0.

### `test_double_mul__0.c` — double multiplication

```c
int main(void) {
    double x = 3.0;
    double y = 7.0;
    double z = x * y;
    return (int)z - 21;
}
```

Expected exit: 0.

### `test_float_div__0.c` — float division

```c
int main(void) {
    float a = 10.0f;
    float b = 4.0f;
    float c = a / b;
    return (int)(c * 10.0f) - 25;   /* 2.5 * 10 = 25 */
}
```

Expected exit: 0.

### `test_double_sub__0.c` — double subtraction

```c
int main(void) {
    double a = 100.0;
    double b = 58.0;
    return (int)(a - b) - 42;
}
```

Expected exit: 0.

### `test_fp_unary_minus__0.c` — unary minus

```c
int main(void) {
    float f = -3.0f;
    double d = -f;    /* double -(-3.0f) = 3.0 */
    return (int)d - 3;
}
```

Expected exit: 0.

### `test_fp_int_widen__0.c` — implicit int → double in expression

```c
int main(void) {
    int n = 7;
    double d = n + 0.5;   /* int 7 widened to double */
    return (int)(d * 2.0) - 15;
}
```

Expected exit: 0.

### `test_fp_cast_roundtrip__0.c` — int → double → int cast

```c
int main(void) {
    int a = 42;
    double d = (double)a;
    int b = (int)d;
    return b - 42;
}
```

Expected exit: 0.

### `test_fp_mixed_expr__0.c` — float/double UAC

```c
int main(void) {
    float  f = 2.0f;
    double d = 3.0;
    double r = f + d;   /* float widened to double */
    return (int)r - 5;
}
```

Expected exit: 0.

---

## Docs (at stage close, after all tests pass)

- **`README.md`** — update "Through stage …" line; note FP arithmetic and
  casts are supported; refresh test totals.
- **`docs/outlines/checklist.md`** — add Stage 110 section.
- Run the **`update-supplemental-docs`** skill.
- **`docs/self-compilation-report.md`** — record Stage 110 self-host run.
