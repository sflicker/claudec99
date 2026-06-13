# ClaudeC99 Stage 111 — `float`/`double` Comparisons and Boolean Contexts

## Goal

Enable float/double values in comparison expressions, logical operators,
and control-flow conditions:

```c
double x = 1.5, y = 2.5;
if (x < y)    { /* taken */ }
while (x > 0) { x -= 0.5; }
int eq = (x == y);    /* 0 */
int ne = (x != y);    /* 1 */
```

Prerequisites: Stages 109 (types/literals) and 110 (arithmetic) must
be complete.

---

## Background — current state after Stage 110

After Stage 110, FP arithmetic expressions produce correct results in
`xmm0`. Using an FP value in a condition (`if`, `while`, `?:`) or
comparison (`<`, `==`) still falls through to the integer comparison
path, which tests `rax` — comparing whatever integer happens to be there
rather than the FP value in `xmm0`. This stage fixes all such paths.

---

## C99 references

| Feature | C99 section |
|---------|-------------|
| Relational and equality operators on FP | §6.5.8, §6.5.9 |
| FP comparison semantics (IEEE 754) | §6.5.8p6 |
| Boolean context (scalar → `_Bool`) | §6.3.1.2 |
| Logical NOT on FP | §6.5.3.3 |

---

## Task 1 — FP comparison instructions (`src/codegen.c`)

### 1a — Instruction selection

The SSE2 unordered compare instructions set CPU flags directly:

```nasm
ucomiss xmm0, xmm1    ; compare float  (left=xmm0, right=xmm1)
ucomisd xmm0, xmm1    ; compare double (left=xmm0, right=xmm1)
```

Flags set by `ucomiss`/`ucomisd`:
- **ZF=1, PF=0, CF=0**: `xmm0 == xmm1` (ordered equal).
- **ZF=0, PF=0, CF=1**: `xmm0 < xmm1` (ordered less-than).
- **ZF=0, PF=0, CF=0**: `xmm0 > xmm1` (ordered greater-than).
- **ZF=1, PF=1, CF=1**: unordered (at least one operand is NaN).

### 1b — `set*` patterns per comparison operator

| C operator | `set*` sequence | Notes |
|-----------|-----------------|-------|
| `<`  | `setb al` | CF=1 |
| `<=` | `setbe al` | CF=1 or ZF=1 |
| `>`  | `seta al` | CF=0 and ZF=0 |
| `>=` | `setae al` | CF=0 |
| `==` | `sete al` ; `setnp cl` ; `and al, cl` | equal AND not NaN |
| `!=` | `setne al` ; `setp cl` ; `or al, cl`  | not-equal OR NaN |

For `==`: a NaN comparison must return 0 (C99 §6.5.9p4: any comparison
involving NaN returns false/zero except `!=`). `sete` alone would return 1
for two NaN values (ZF=1 in the unordered case), so AND with `setnp`
(not-parity = not NaN) is required.

For `!=`: C99 §6.5.9p4: `NaN != NaN` is true. `setne` alone would return 0
for `NaN != NaN` (ZF=1 in the unordered case), so OR with `setp` is
required.

After the `set*` sequence, `movzx rax, al` to zero-extend the 1/0 byte
into the full result register.

### 1c — Operand loading convention

Evaluate the left operand into `xmm0`; save to stack; evaluate the right
operand into `xmm0`; restore left into `xmm1`; then emit `ucomiss xmm0,
xmm1`. This places left in `xmm1` and right in `xmm0` after the restore,
which is the correct argument order for the instructions: the first operand
to `ucomiss`/`ucomisd` is the left-hand side of the C comparison.

Wait — the NASM `ucomiss dst, src` compares `dst` (left) against `src`
(right): result flags reflect `dst cmp src`. So:

```nasm
ucomiss xmm1, xmm0    ; compares left (xmm1) against right (xmm0)
```

Then `setb al` means "left < right" (CF set when left < right after
`ucomiss left, right`). Keep the instruction order consistent with the
save/restore convention established in Stage 110.

---

## Task 2 — FP in boolean contexts (`src/codegen.c`)

Any scalar value used as a condition in C is "true" if non-zero. For FP:
a zero value (including `-0.0`) is false; any non-zero value (including
NaN — see note) is true.

**NaN policy**: C99 treats NaN comparisons as false for `<`, `<=`, `>`,
`>=`, `==`, and true for `!=`. For a bare boolean context (`if (nan_val)`)
the standard says the result is unspecified when the condition is NaN.
ClaudeC99 treats NaN as true in boolean context (by using `setne + setp`
pattern), which is one valid implementation choice.

**Codegen** for an FP value in a condition slot:

```nasm
; value in xmm0
xorpd xmm1, xmm1              ; zero in xmm1 (for double)
ucomisd xmm0, xmm1            ; compare value against 0.0
setne al                      ; non-zero?
setp  cl                      ; NaN? (NaN is also "true" here)
or    al, cl
movzx rax, al                 ; rax = 1 if (value != 0.0 || NaN), else 0
```

For `float`:

```nasm
xorps xmm1, xmm1
ucomiss xmm0, xmm1
setne al
setp  cl
or    al, cl
movzx rax, al
```

This value in `rax` feeds into the existing integer condition-code testing
used by `if`/`while`/etc.

---

## Task 3 — Logical NOT on FP (`src/codegen.c`)

`!fp_value` returns 1 if the value is zero (and not NaN), 0 otherwise.
Per C99, `!NaN` = 0.

```nasm
; value in xmm0 (double)
xorpd xmm1, xmm1
ucomisd xmm0, xmm1
sete  al           ; equal to zero?
setnp cl           ; not NaN?
and   al, cl       ; zero AND not NaN
movzx rax, al
```

---

## Task 4 — Mixed FP/int comparisons

When comparing a float/double against an integer expression, the integer
must be converted to FP before the comparison (C99 §6.3.1.8):

```nasm
cvtsi2sd xmm1, rax    ; int → double before ucomisd
```

The type resolution for `AST_BINARY_OP` nodes (already extended for
arithmetic in Stage 110) should already promote the integer side; the
same conversion codegen path applies here.

---

## Task 5 — Version bump (`src/version.c`)

```c
#define VERSION_STAGE  "01110000"
```

---

## Implementation order

1. `src/codegen.c` — `ucomiss`/`ucomisd` comparison codegen (Task 1)
2. `src/codegen.c` — FP boolean context (Task 2)
3. `src/codegen.c` — logical NOT on FP (Task 3)
4. `src/codegen.c` — mixed FP/int comparison conversions (Task 4)
5. `src/version.c` — version bump
6. Tests
7. Build, full test suite, self-host cycle

---

## Out of scope — do NOT do these in this stage

- **FP function parameters and return values** — Stage 112.
- **`isnan()` / `isinf()`** — library functions; Stage 112.
- **Floating-point exceptions (`fenv.h`)** — deferred.
- **`long double`** — deferred.

---

## Tests

### `test_fp_less_than__0.c` — `<` comparison

```c
int main(void) {
    double a = 1.0, b = 2.0;
    return (a < b) ? 0 : 1;
}
```

Expected exit: 0.

### `test_fp_equal__0.c` — `==` comparison

```c
int main(void) {
    float x = 4.0f;
    float y = 2.0f + 2.0f;
    return (x == y) ? 0 : 1;
}
```

Expected exit: 0.

### `test_fp_not_equal__0.c` — `!=` comparison

```c
int main(void) {
    double a = 1.5, b = 2.5;
    return (a != b) ? 0 : 1;
}
```

Expected exit: 0.

### `test_fp_if_condition__0.c` — FP value in `if` condition

```c
int main(void) {
    double x = 3.14;
    if (x) return 0;
    return 1;
}
```

Expected exit: 0.

### `test_fp_while_loop__0.c` — FP counter in `while` loop

```c
int main(void) {
    double sum = 0.0;
    double i   = 1.0;
    while (i <= 10.0) {
        sum = sum + i;
        i   = i + 1.0;
    }
    return (int)sum - 55;   /* 1+2+...+10 = 55 */
}
```

Expected exit: 0.

### `test_fp_logical_not__0.c` — `!` on FP value

```c
int main(void) {
    double zero    = 0.0;
    double nonzero = 1.5;
    int a = !zero;      /* 1 */
    int b = !nonzero;   /* 0 */
    return (a == 1 && b == 0) ? 0 : 1;
}
```

Expected exit: 0.

### `test_fp_ternary__0.c` — FP value in ternary condition

```c
int main(void) {
    double x = 0.5;
    int r = x > 0.0 ? 42 : -1;
    return r - 42;
}
```

Expected exit: 0.

### `test_fp_mixed_cmp__0.c` — FP compared against integer expression

```c
int main(void) {
    double d = 5.0;
    int    n = 3;
    return (d > n) ? 0 : 1;   /* 5.0 > 3 → true */
}
```

Expected exit: 0.

---

## Docs (at stage close, after all tests pass)

- **`README.md`** — update "Through stage …" line; note FP comparisons and
  control-flow are supported; refresh test totals.
- **`docs/outlines/checklist.md`** — add Stage 111 section.
- Run the **`update-supplemental-docs`** skill.
- **`docs/self-compilation-report.md`** — record Stage 111 self-host run.
