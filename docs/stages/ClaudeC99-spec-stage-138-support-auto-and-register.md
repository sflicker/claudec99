# ClaudeC99 Stage 138 - support register and auto storage classes.

This report covers two C99 storage-class specifiers tested with:

```text
/home/scott/code/claude/claudec99/bin/cc99
```

The reduced examples were compared against GCC using
`gcc -std=c99 -O0 -Wall -Wextra -pedantic`. GCC accepted both programs and
returned the expected values.

## Summary

| ID | Area | Symptom | Priority |
|----|------|---------|----------|
| CC99-011 | Parser / declaration specifiers | Explicit `auto` storage class is rejected before the declared type | Medium |
| CC99-012 | Parser / declaration specifiers | `register` storage class is parsed as an unknown type name | Medium |

## CC99-011: Explicit Auto Storage Class Is Rejected

### Reduced Source

```c
int compute(int seed) {
    auto int value = seed + 3;
    auto int values[3] = { 2, 4, 6 };
    auto int i;
    auto int total = value;

    for (i = 0; i < 3; i = i + 1) {
        total = total + values[i];
    }

    {
        auto int inner = total + 5;
        total = inner;
    }

    return total;
}

int main(void) {
    return compute(7);
}
```

### Compile Commands

```sh
/home/scott/code/claude/claudec99/bin/cc99 -o /tmp/test_auto_storage_class_cc99 cc99_testing/test_auto_storage_class.c
gcc -std=c99 -O0 -Wall -Wextra -pedantic -o /tmp/test_auto_storage_class_gcc cc99_testing/test_auto_storage_class.c
```

### Observed Behavior

GCC accepts the program and returns `27`.

cc99 rejects the first explicit `auto` declaration:

```text
/home/scott/code/c/cc99_benchmarks_and_testing/cc99_testing/test_auto_storage_class.c:2:18: error: expected ';', got 'int' ('int')
```

### Expected Behavior

C99 permits `auto` as an explicit storage-class specifier for block-scope
objects. It does not change the generated behavior compared with the default
automatic storage duration. The reduced program should compile and return `27`.

## CC99-012: Register Storage Class Is Rejected

### Reduced Source

```c
int compute(register int seed) {
    register int value = seed + 3;
    register int i;
    int values[3] = { 2, 4, 6 };
    register int total = value;

    for (i = 0; i < 3; i = i + 1) {
        total = total + values[i];
    }

    {
        register int inner = total + 5;
        total = inner;
    }

    return total;
}

int main(void) {
    return compute(7);
}
```

### Compile Commands

```sh
/home/scott/code/claude/claudec99/bin/cc99 -o /tmp/test_register_storage_class_cc99 cc99_testing/test_register_storage_class.c
gcc -std=c99 -O0 -Wall -Wextra -pedantic -o /tmp/test_register_storage_class_gcc cc99_testing/test_register_storage_class.c
```

### Observed Behavior

GCC accepts the program and returns `27`.

cc99 rejects `register` in the parameter declaration:

```text
/home/scott/code/c/cc99_benchmarks_and_testing/cc99_testing/test_register_storage_class.c:1:13: error: unknown type name 'register'
```

### Expected Behavior

C99 permits `register` as a storage-class specifier for block-scope object
declarations and function parameters. For code generation it can be treated as
a non-binding allocation hint, with the important semantic restriction that the
address of a `register` object cannot be taken. This reduced program does not
take the address of any `register` object, so it should compile and return
`27`.

### Likely Fix Area

Declaration-specifier parsing appears not to recognize `auto` or `register` as
storage-class specifiers. Add both tokens to the storage-class specifier path
and carry the resulting storage-class information through semantic analysis.

For initial support:

- `auto` can be accepted only for block-scope object declarations and lowered
  exactly like the default automatic storage class.
- `register` can be accepted for block-scope object declarations and function
  parameters, lowered like an automatic object for allocation purposes, and
  marked so address-of can be diagnosed if semantic checks cover that rule.
- Both specifiers should be rejected in invalid contexts, such as file-scope
  `auto` objects.

### Regression Tests To Add

- Explicit `auto int x;` at block scope
- `auto` arrays and initialized block-scope objects
- `register int x;` at block scope
- `register` function parameters
- Diagnostic for taking the address of a `register` object
- Diagnostic for invalid file-scope `auto`
