# ClaudeC99 stage 135

## Task fix the following bug report

## cc99 Bug Report

This report covers C99 type compatibility and composite type checks tested with:

```text
/home/scott/code/claude/claudec99/bin/cc99
```

The reduced examples were compared against GCC using
`gcc -std=c99 -O0 -Wall -Wextra -pedantic`. GCC accepted the failing examples
below. GCC warns about mismatched array-parameter spelling, but the declarations
are still compatible after C function-parameter adjustment.

Related checks that passed in cc99:

- Incomplete external array completed by a later definition
- Old-style no-prototype function declaration followed by a prototype
- Function parameter redeclaration differing only by a top-level `const`
  qualifier

## Summary

| ID | Area | Symptom | Priority |
|----|------|---------|----------|
| CC99-008 | Type compatibility / function parameters | Compatible array-parameter and pointer-parameter redeclarations are rejected | Medium |
| CC99-009 | Type parser / composite type coverage | Pointer-to-array parameter types are rejected before composite type rules can apply | Medium |

## CC99-008: Array Parameters Are Not Compared After Parameter Adjustment

### Reduced Source

```c
int sum_edges(int values[3]);
int sum_edges(int *values);

int sum_edges(int values[]) {
    return values[0] + values[2];
}

int main(void) {
    int values[3] = { 2, 5, 7 };

    return sum_edges(values);
}
```

### Compile Commands

```sh
/home/scott/code/claude/claudec99/bin/cc99 -o /tmp/test_composite_array_parameter_cc99 cc99_testing/test_composite_array_parameter.c
gcc -std=c99 -O0 -Wall -Wextra -pedantic -o /tmp/test_composite_array_parameter_gcc cc99_testing/test_composite_array_parameter.c
```

### Observed Behavior

GCC accepts the program and returns `9`.

cc99 rejects the second declaration:

```text
/home/scott/code/c/cc99_benchmarks_and_testing/cc99_testing/test_composite_array_parameter.c:2:27: error: function 'sum_edges' parameter type mismatch at position 1
```

### Expected Behavior

In a function parameter declaration, `int values[3]`, `int values[]`, and
`int *values` all declare an adjusted parameter type of `int *`. These
declarations are compatible and should form the same function type. The reduced
program should compile and return `9`.

### Likely Fix Area

Function redeclaration compatibility appears to compare the written parameter
declarator forms before applying C parameter adjustment. Normalize parameter
types before comparing function declarations:

- Array parameter declarations adjust to pointer-to-element type
- Function parameter declarations adjust to pointer-to-function type
- Top-level qualifiers on value parameters do not affect compatibility

### Regression Tests To Add

- Prototype `int f(int a[3]);` followed by `int f(int *a);`
- Prototype `int f(int a[]);` followed by a definition `int f(int *a)`
- Array parameter with a named bound followed by an unbounded array parameter
- Function parameter adjustment, for example `int f(int cb(void));` and
  `int f(int (*cb)(void));`

## CC99-009: Pointer-To-Array Types Are Not Supported

### Reduced Source

```c
int tail(int (*row)[]);

int tail(int (*row)[4]) {
    return (*row)[3];
}

int main(void) {
    int row[4] = { 1, 2, 3, 13 };

    return tail(&row);
}
```

### Compile Commands

```sh
/home/scott/code/claude/claudec99/bin/cc99 -o /tmp/test_composite_pointer_array_parameter_cc99 cc99_testing/test_composite_pointer_array_parameter.c
gcc -std=c99 -O0 -Wall -Wextra -pedantic -o /tmp/test_composite_pointer_array_parameter_gcc cc99_testing/test_composite_pointer_array_parameter.c
```

### Observed Behavior

GCC accepts the program and returns `13`.

cc99 rejects the first declaration before reaching redeclaration compatibility:

```text
/home/scott/code/c/cc99_benchmarks_and_testing/cc99_testing/test_composite_pointer_array_parameter.c:1:20: error: pointer to array types are not supported
```

### Expected Behavior

C99 permits pointers to array types. A declaration using pointer to array of
unknown bound is compatible with a later declaration using pointer to array of a
known bound when the element types are compatible, and the composite type uses
the known bound. The reduced program should compile and return `13`.

### Likely Fix Area

The type parser or semantic layer has an explicit unsupported path for
pointer-to-array types. Supporting this type form is required before cc99 can
implement composite type rules involving incomplete array types behind pointers.

### Regression Tests To Add

- Pointer to known-bound array parameter: `int f(int (*row)[4]);`
- Pointer to unknown-bound array declaration completed by known-bound
  redeclaration
- Indexed access through `(*row)[i]`
- Incompatible pointer-to-array bounds where both declarations have different
  known constant sizes
