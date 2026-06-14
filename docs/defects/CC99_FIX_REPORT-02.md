# cc99 Bug Report

This report lists current cc99 bugs found by compiling and running standalone
C99 programs with:

```text
/home/scott/code/claude/claudec99/bin/cc99
```

Each section includes source code intended to be copied into a small `.c` file
and compiled directly with cc99. The examples have defined behavior and were
compared against GCC/Clang where relevant.

## Summary

| ID | Area | Symptom | Priority |
|----|------|---------|----------|
| CC99-001 | SysV AMD64 ABI / FP arguments | Stack-passed non-variadic `double` arguments are ignored after `xmm0`-`xmm7` | High |
| CC99-002 | SysV AMD64 ABI / variadic FP arguments | `va_arg(ap, double)` fails for the 9th and later double arguments | High |
| CC99-003 | Indirect calls / FP arguments | Function-pointer calls with multiple `double` arguments produce wrong results | High |
| CC99-004 | Constant expressions / pointer initializers | Valid static pointer and function-pointer initializers are rejected | Medium |
| CC99-005 | FP logical operators | `&&` and `||` on `double` operands do not short-circuit RHS side effects | High |

## CC99-001: Non-Variadic Double Arguments Beyond 8 Are Not Passed Correctly

### Reduced Source

```c
/* SysV AMD64 passes the first 8 double arguments in xmm0-xmm7.
   The 9th double argument is stack-passed and should still be visible. */
double sum9(
    double a1, double a2, double a3,
    double a4, double a5, double a6,
    double a7, double a8, double a9
) {
    return a1 + a2 + a3 + a4 + a5 + a6 + a7 + a8 + a9;
}

int main(void) {
    return (int)sum9(1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0);
}
```

### Compile Commands

```sh
/home/scott/code/claude/claudec99/bin/cc99 -o /tmp/repro_fp_arg9_cc99 repro_fp_arg9.c
gcc -std=c99 -O0 -o /tmp/repro_fp_arg9_gcc repro_fp_arg9.c
```

### Observed Behavior

- GCC returns `45`.
- cc99 returns `36`.

cc99 appears to sum only the first 8 arguments and ignore the 9th stack-passed
`double`.

### Expected Behavior

cc99 should return `45`.

### Additional Observations

The same pattern appears with 10, 11, and 12 `double` parameters. cc99 returns
`36` for each case, suggesting all stack-passed FP parameters are missing.

Mixed integer and FP parameters show the same FP overflow issue:

```c
/* The 6 integer args fit in GP registers. The 9th double is stack-passed. */
double f(
    int a, int b, int c, int d, int e, int f,
    double x1, double x2, double x3, double x4, double x5,
    double x6, double x7, double x8, double x9
) {
    return (double)(a + b + c + d + e + f)
        + x1 + x2 + x3 + x4 + x5 + x6 + x7 + x8 + x9;
}

int main(void) {
    return (int)f(1,2,3,4,5,6, 1.0,2.0,3.0,4.0,5.0,6.0,7.0,8.0,9.0);
}
```

Expected result: `66`.
Observed cc99 result: `57`.

### Likely Fix Area

Inspect call lowering and function prologue handling for FP parameters after the
8 XMM register arguments. The stack overflow area for non-variadic FP arguments
likely is not being loaded into local parameter slots.

## CC99-002: Variadic Double Arguments Beyond 8 Are Not Read Correctly

### Reduced Source

```c
/* Variadic double arguments after xmm0-xmm7 must be read from the overflow
   argument area by va_arg(ap, double). */
#include <stdarg.h>

int sumd(int n, ...) {
    va_list ap;
    int i;
    double s = 0.0;

    va_start(ap, n);
    for (i = 0; i < n; i = i + 1) {
        s = s + va_arg(ap, double);
    }
    va_end(ap);

    return (int)s;
}

int main(void) {
    return sumd(9, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0);
}
```

### Compile Commands

```sh
/home/scott/code/claude/claudec99/bin/cc99 -I /home/scott/code/claude/claudec99/test/include -o /tmp/repro_vararg_double9_cc99 repro_vararg_double9.c
gcc -std=c99 -O0 -o /tmp/repro_vararg_double9_gcc repro_vararg_double9.c
```

### Observed Behavior

- GCC returns `45`.
- cc99 returns `36`.

### Expected Behavior

cc99 should return `45`.

### Likely Fix Area

Inspect `va_arg(ap, double)` handling when `fp_offset` has passed the 8 XMM
register slots and the next double must come from `overflow_arg_area`.

## CC99-003: Function-Pointer Calls With Double Arguments Produce Wrong Results

### Reduced Source A: Two Double Arguments

```c
/* Direct calls with two double arguments work, but the same call through a
   function pointer returns the wrong result. */
typedef double (*Fn)(double, double);

double add2(double a, double b) {
    return a + b;
}

int main(void) {
    Fn p = add2;
    return (int)p(1.0, 2.0);
}
```

Expected result: `3`.
Observed cc99 result: `2`.

### Reduced Source B: Three Double Arguments

```c
/* With three double arguments, cc99 returned 0 in testing. */
typedef double (*Fn)(double, double, double);

double add3(double a, double b, double c) {
    return a + b + c;
}

int main(void) {
    Fn p = add3;
    return (int)p(1.0, 2.0, 3.0);
}
```

Expected result: `6`.
Observed cc99 result: `0`.

### Expected Behavior

Indirect function calls should use the same argument classification and register
assignment as direct calls. The examples above should return `3` and `6`.

### Likely Fix Area

Inspect indirect-call codegen for floating-point arguments. The direct call path
may be assigning XMM arguments correctly while the function-pointer call path is
not moving all FP arguments into the expected XMM registers before `call *reg`.

## CC99-004: Static Pointer Initializers Are Rejected As Non-Constant

### Reduced Source A: Global Pointer To Global Object

```c
/* The address of a file-scope object is a valid constant initializer for a
   file-scope pointer object in C. */
int x = 37;
int *p = &x;

int main(void) {
    return *p;
}
```

Expected result: compile and return `37`.
Observed cc99 compile error:

```text
error: non-constant initializer for global 'p'
```

### Reduced Source B: Global Pointer To Array Element

```c
/* Address constants may include an array element designator. */
int xs[3] = { 5, 7, 11 };
int *p = &xs[2];

int main(void) {
    return *p;
}
```

Expected result: compile and return `11`.
Observed cc99 compile error:

```text
error: non-constant initializer for global 'p'
```

### Reduced Source C: Global Function Pointer

```c
/* Function designators are valid constant initializers for function pointers. */
typedef int (*Fn)(int, int);

int add(int a, int b) {
    return a + b;
}

Fn fp = add;

int main(void) {
    return fp(8, 9);
}
```

Expected result: compile and return `17`.
Observed cc99 compile error:

```text
error: non-constant initializer for global 'fp'
```

### Reduced Source D: Block-Scope Static Pointer

```c
/* The same address constant rule applies to block-scope static initializers. */
int x = 51;

int main(void) {
    static int *p = &x;
    return *p;
}
```

Expected result: compile and return `51`.
Observed cc99 compile error:

```text
error: initializer for block-scope static 'p' must be a constant expression
```

### Likely Fix Area

The constant-initializer evaluator appears to handle integer constant
expressions but not address constants. Add support for C address constants in
file-scope and block-scope static initializers:

- `&global_object`
- `&global_array[index]`
- Function designators such as `fp = add`
- `&function`
- Optional null pointer arithmetic forms if supported elsewhere

## CC99-005: Floating-Point Logical Operators Do Not Short-Circuit Side Effects

### Reduced Source

```c
/* For &&, the RHS must not be evaluated when the left operand is false.
   For ||, the RHS must not be evaluated when the left operand is true.
   This should hold for double operands after conversion to boolean. */
int main(void) {
    double a = 0.0;
    double b = 2.0;
    int x = 0;

    if (a && (x = 1)) {
        x = 2;
    }

    if (b || (x = 3)) {
        x = x + 5;
    }

    return x;
}
```

### Compile Commands

```sh
/home/scott/code/claude/claudec99/bin/cc99 -o /tmp/repro_fp_short_circuit_cc99 repro_fp_short_circuit.c
gcc -std=c99 -O0 -o /tmp/repro_fp_short_circuit_gcc repro_fp_short_circuit.c
```

### Observed Behavior

- GCC returns `5`.
- cc99 returns `8`.

The cc99 result is consistent with evaluating both RHS assignments:

- `(x = 1)` is evaluated even though `a` is `0.0`.
- `(x = 3)` is evaluated even though `b` is nonzero.

### Expected Behavior

cc99 should return `5`.

### Likely Fix Area

The FP boolean conversion path for logical `&&` and `||` appears to compute
truth values without preserving C short-circuit evaluation. Logical operator
codegen should branch after evaluating the left operand before emitting the RHS,
just as integer logical operators do.
