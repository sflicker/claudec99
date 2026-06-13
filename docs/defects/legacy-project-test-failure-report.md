# Legacy Project Test Failure Report

**Date**: 2026-06-13  
**Suite**: `test/valid/legacy_project/` (219 tests)  
**Result**: 195 passed, **24 failed**  
**Compiler**: ClaudeC99 stage 113

---

## Summary

| Category | Count |
|----------|-------|
| Test file naming error (exit code out of range) | 3 |
| Test file coding error (invalid C99) | 1 |
| Missing compiler feature | 2 |
| Compiler bug — nested brace multidim array initializers | 7 |
| Compiler bug — FP array subscript write/read | 10 |
| Compiler bug — ternary with mixed FP/int branches | 1 |
| **Total failures** | **24** |

---

## Category 1: Test file naming error — exit code out of range

Linux exit codes are captured via `$?`, which holds only the low 8 bits of the exit
status (range 0–255). Tests whose `__N` filename encodes a negative or >255 value
will always fail because the runner compares the raw filename number against the
8-bit-truncated actual exit code.

These programs likely behave **correctly**; only the filename expectation is wrong.

### `expressions/test_unary_neg__-42.c`

```c
int main() { return -42; }
```

**Failure**: expected=-42, got=214  
**Diagnosis**: The program correctly returns -42. The OS wraps negative exit codes to
unsigned byte: `(-42) & 0xFF = 214`. The filename should encode `214`, not `-42`.

---

### `floating_point/test_trunc_toward_zero__-3.c`

```c
int main() {
    double x = -3.9;
    return (int)x;         // -3
}
```

**Failure**: expected=-3, got=253  
**Diagnosis**: `(int)-3.9` truncates toward zero to `-3`. The OS wraps `-3` to
`(-3) & 0xFF = 253`. The program is correct; the filename should encode `253`.

---

### `types/test_char_promotion__260.c`

```c
int main() {
    char c = 250;
    return c + 10;
}
```

**Failure**: expected=260, got=4  
**Diagnosis**: Two compounding issues:

1. On x86-64 Linux, `char` is **signed**. Assigning 250 (> CHAR_MAX=127) to a signed
   char yields implementation-defined behavior; in practice it wraps to -6. So
   `c + 10 = -6 + 10 = 4`. If the test assumed unsigned char, the code itself is
   non-portable.

2. Even if char were unsigned (c=250, c+10=260), exit codes are 8-bit, so
   `260 & 0xFF = 4`. A value >255 can never be distinguished from its truncated form.

**Fix needed**: Use `unsigned char` and stdout comparison if the value 260 must be
verified, or adjust the expected exit code to 4.

---

## Category 2: Test file coding error

### `array/test_long_array__42.c`

```c
int main() {
    long A[10] = {0,1,42,3,1,2,3,4,5,6,7};
    return (int)A[2];
}
```

**Failure**: compiler error — `too many initializers for array 'A'`  
**Diagnosis**: The initializer list has **11 elements** but the array is declared with
size 10. This is a C99 constraint violation (excess initializers). GCC and Clang both
reject it. The test file has a coding error — one initializer must be removed. With
the extra element removed, the program should return `A[2] = 42`.

---

## Category 3: Missing compiler feature

### `global/test_global_array_with_nested_init__42.c`

```c
int A[2][2] = {{36, 1},
               {2, 3}};

int main() {
    int sum = 0;
    sum += A[0][0]; sum += A[0][1];
    sum += A[1][0]; sum += A[1][1];
    return sum;
}
```

**Failure**: compiler error — `unsupported initializer element type in 'A'`  
**Diagnosis**: The compiler does not support **nested brace initializers for global
multidimensional arrays**. File-scope array initialization only handles flat brace
lists (`{36, 1, 2, 3}`), not sub-brace-grouped forms (`{{36, 1}, {2, 3}}`).  
This is a valid C99 program. The feature is not yet implemented.

---

### `string/test_string_expression__99.c`

```c
int main() {
    return "abc"[2];       // should return 'c' = 99
}
```

**Failure**: compiler error — `subscript base must be an identifier`  
**Diagnosis**: The compiler requires the subscript base to be an identifier (a named
variable), not an arbitrary expression. Subscripting a **string literal directly**
(`"abc"[2]`) is valid C99 (the literal decays to a pointer) but is not yet supported.  
Workaround: `const char *s = "abc"; return s[2];` would work.

---

## Category 4: Compiler bug — local multidimensional array nested brace initialization

All seven tests use **nested (sub-brace-grouped) initializers** for local multidim
arrays, e.g. `int A[2][3] = {{36, 1, 0}, {2, 3, 0}}`. The compiler accepts the
syntax without error but silently discards the inner brace values. The array is
correctly zero-initialized but the initializer values are never stored, producing
incorrect sums.

Confirmed by assembly inspection: the initializer code emits `mov byte [rbp - N], 0`
(zero-fill) and then an uninitialized `movsxd rax, eax` (reading a stale register),
meaning the initializer expression is either being parsed as a scalar (single outer
value) or completely dropped.

### Affected tests

| Test | Declared | Expected | Got |
|------|----------|----------|-----|
| `array/test_1_by_2_array__42.c` | `int A[1][2] = {{36, 6}}` | 42 | 16 |
| `array/test_2_by_1_array__42.c` | `int A[2][1] = {{36}, {6}}` | 42 | 32 |
| `array/test_2_by_3_array__42.c` | `int A[2][3] = {{36,1,0},{2,3,0}}` | 42 | 32 |
| `array/test_3_by_4_array__42.c` | `int A[3][4] = {{36,1,0,1},{2,3,0,-1},{-10,20,-10,0}}` | 42 | 48 |
| `array/test_local_multi_with_nested__42.c` | `int A[2][2] = {{36,1},{2,3}}` | 42 | 32 |
| `array/test_multi_array_index_via_pointer__20.c` | `int a[3][2] = {{2,20},...}; int *p = &a[0][0]; return *(p+1)` | 20 | 0 |
| `array/test_multi_array_index_via_pointer_with_cast__20.c` | `int a[3][2] = {{2,20},...}; int *p = (int*)a; return *(p+1)` | 20 | 0 |

The pointer tests return 0 (not 20) because `a[0][1]` is 0 due to the initializer
being dropped — they would work correctly if the initializer bug were fixed.

**Note**: Flat multidimensional initializers (no inner braces) also appear to be
unsupported for local arrays — the compiler rejects `int A[2][2] = {36,1,2,3}` with
"too many initializers". Nested-brace sub-grouping is the standard C99 form and
should be treated as a single bug.

---

## Category 5: Compiler bug — FP array subscript write/read

All ten tests involve **writing to or reading from elements of a `double[]` or `float[]`
array using a subscript index**. The compiler produces incorrect code at both the write
and read site.

**Write bug** (confirmed by assembly): after computing the address of `a[i]` into `rbx`
and loading the FP value into `xmm0`, the compiler emits `mov [rbx], rax` (stores the
old value of `rax`, not the FP result). It should emit `movsd [rbx], xmm0` (for
double) or `movss [rbx], xmm0` (for float).

**Read bug**: after loading the 8-byte slot via `mov rax, [rax]`, the compiler emits
`movsxd rax, eax`, which sign-extends only the low 32 bits and discards the high 32
bits of the IEEE 754 bit pattern. This truncates the double's exponent and mantissa.

The combination makes FP array elements unreadable, producing garbage or 0 for
multi-element arrays.

Single-element access (`a[0]`) was tested separately and gave the correct result in one
case, so the bug may be partly masked by coincidental register state at index 0.

### Affected tests

| Test | Description | Got |
|------|-------------|-----|
| `floating_point/test_dbl_array_sum__42.c` | `double a[5]` assigned, summed in loop with `+=` | 0 |
| `floating_point/test_dbl_array_sum2__42.c` | same array, one-line chain sum | 24 |
| `floating_point/test_dbl_array_sum3__42.c` | same array, sequential `+=` | 0 |
| `floating_point/test_dbl_array_sum4__42.c` | same array, sequential `= sum + a[i]` | 0 |
| `floating_point/test_dbl_with_initializer_list__42.c` | `double a[5] = {1.0, ...}`, loop `+=` | 0 |
| `floating_point/test_dbl_with_initializer_list2__42.c` | brace-init, loop `= sum + a[i]` | 0 |
| `floating_point/test_dbl_with_initializer_list3__42.c` | brace-init, one-line chain sum | 80 |
| `floating_point/test_fp_with_initializer_list__42.c` | `float a[5] = {1.0f,...}`, loop `+=` | 0 |
| `floating_point/test_fp_with_initializer_list2__42.c` | float brace-init, loop `= sum + a[i]` | 0 |
| `floating_point/test_fp_with_initializer_list3__42.c` | float brace-init, one-line chain sum | 80 |

---

## Category 6: Compiler bug — ternary operator with mixed FP/int branches

### `expressions/test_conditional_expression_with_casts2__42.c`

```c
int main() {
    int a = 1;
    float b = 42;
    int c = 24;
    double d = a ? b : c;
    return d;
}
```

**Failure**: expected=42, got=1

**Diagnosis** (confirmed by assembly): The compiler generates the following at the
merge point of the ternary:

```asm
; True branch taken (a==1):
movss xmm0, [rbp - 8]    ; xmm0 = b = 42.0f   ← correct
jmp .L_cond_end_0

; False branch:
mov eax, [rbp - 12]       ; eax = c = 24       ← correct

.L_cond_end_0:
cvtsi2sd xmm0, rax        ; ALWAYS converts rax to double, overwriting xmm0!
```

When the true branch is taken, `xmm0` correctly holds 42.0f, but `rax` still contains
1 (the value of `a` from the condition test). The merge point unconditionally applies
`cvtsi2sd xmm0, rax`, which overwrites the correct float result with the integer
conversion of `rax` (= 1 → 1.0). The final `return d` converts 1.0 to int → exit
code 1.

**Root cause**: the code generator does not distinguish between the two branches at
the merge point when one branch produces an FP result in xmm0 and the other produces
an int result in rax. It emits a single unconditional integer-to-FP conversion.

**Fix**: the true branch should also place its result in a canonical register/type,
or the merge point must emit a conditional conversion. A simpler fix is for the
code generator to recognize that the true branch already produced the result in xmm0
and skip the `cvtsi2sd` on that path.

---

## Runnable status

All 24 failing tests are runnable (the compiler either accepts the input and runs it,
or produces a clear diagnostic). No tests were skipped due to missing `__N` suffix or
other harness issues — the runner handled all files correctly.

The 3 exit-code-range tests and 1 over-initialized array test have issues in the test
files themselves. The remaining 20 tests expose compiler limitations.
