# ClaudeC99 Stage 137 - function return function pointers

This report covers a C99 declarator form tested with:

```text
/home/scott/code/claude/claudec99/bin/cc99
```

The reduced example was compared against GCC using
`gcc -std=c99 -O0 -Wall -Wextra -pedantic`. GCC accepted the program and
returned the expected value.

## Summary

| ID | Area | Symptom | Priority |
|----|------|---------|----------|
| CC99-010 | Parser / function declarators | A function returning a function pointer is rejected as unsupported | Medium |

## CC99-010: Functions Returning Function Pointers Are Rejected

### Reduced Source

```c
int add5(int value) {
    return value + 5;
}

int (*get_adder())(int) {
    return add5;
}

int main(void) {
    int (*fn)(int);

    fn = get_adder();

    return fn(7) + get_adder()(11);
}
```

### Compile Commands

```sh
/home/scott/code/claude/claudec99/bin/cc99 -o /tmp/test_function_returning_function_pointer_cc99 cc99_testing/test_function_returning_function_pointer.c
gcc -std=c99 -O0 -Wall -Wextra -pedantic -o /tmp/test_function_returning_function_pointer_gcc cc99_testing/test_function_returning_function_pointer.c
```

### Observed Behavior

GCC accepts the program and returns `28`.

cc99 rejects the function definition:

```text
/home/scott/code/c/cc99_benchmarks_and_testing/cc99_testing/test_function_returning_function_pointer.c:5:16: error: functions returning function pointers are not supported
```

### Expected Behavior

C99 permits a function to return a pointer to function. The declaration:

```c
int (*get_adder())(int)
```

declares `get_adder` as a function with no prototype returning a pointer to a
function that takes an `int` and returns `int`. The reduced program should
compile, allow assigning the returned pointer to `int (*fn)(int)`, allow calling
both `fn(7)` and `get_adder()(11)`, and return `28`.

### Likely Fix Area

The declarator parser or semantic type builder appears to reject function
return types whose derived type is pointer-to-function. This is only invalid
when a function returns a function type directly; returning a pointer to a
function type is valid.

Preserve the distinction between:

- invalid: `int f()(int)` as a direct function-returning-function type
- valid: `int (*f())(int)` as a function returning pointer-to-function

The type checker and code generator also need to treat the returned value as a
normal pointer value that can be assigned, returned, and used as an indirect
call target.

### Regression Tests To Add

- Function definition using `int (*f())(int)` returning a function designator
- Assignment of the returned function pointer to `int (*p)(int)`
- Direct call through the returned pointer, for example `f()(11)`
- Equivalent spelling with a typedef for the returned function-pointer type
