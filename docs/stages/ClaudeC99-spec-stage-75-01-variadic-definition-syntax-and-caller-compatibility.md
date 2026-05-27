# ClaudeC99 Stage 75-01 Variadic function definition syntax and caller compatibility

## Goal
  - Allow ClaudeC99 to parse and type-check variadic function definitions
```C
int f(int fixed, ...) {
    return fixed;
}   
```
Add allow callers to pass more arguments than the named parameter count
```C
f(42,1,2,3);
```
without adding stdarg.h, va_list, va_start, or va_arg or callee-side access to unnamed arguments yet.

## In-Scope
Support
```C
int log_value(const char *fmt, ...) {
    return 42;
}

int main(void) {
    return log_value("%d", 1, 2, 3);
```
The callee ignores the extra arguments, but the program is still valid and testable

Support prototypes and definitions
```C
int f(int x, ...);

int f(int x, ...) {
    return x;
}
```

Function type should record:
is_variadic = true
fixed_param_count = N

**Caller Rule**
For non-variadic functions
```text
actual_arg_count must equal param_count
```

For variadic functions
```text
actual_arg_count must be >= fixed_param_count
```

So
```C
int f(x, ...);

f();       /* invalid: missing fixed arg */
f(1);      /* valid */
f(1,2);    /* valid */
f(1,2,3);  /* valid */
```

Important ABI note
for a call to a variadic function, emit
```asm
xor eax, eax
```
before the call, the same way this is already done for external variadic calls, because on SysV AMD64 %l
carries the number of vector registers used for floating point args. Since ClaudeC99 currently  has no floating
point support, this is always zero.

## codegen for 7+ arguments.
The same rules should apply for variadic functions when the total number of arguments is 7+.
The first six arguments should be placed in registers and the rest on the stack.

## Out of scope
  - callee side handling of variadic arguments
  - stdarg.h
  - va_list
  - va_arg
  - va_end
  - va_copy
  - passing va_list to helper functions
  - register save area


## Tests
```C
int fixed(int f, ...) {
    return f;
}

int main(void) {
    return fixed(42, 1,2,3);      // expect 42    
```

```C
int sum(int a, int b, ...) {
    return a + b;
}

int main(void) {
    reutrn sum(1,2,3,4,5);      // expect 3
```

```C
int log(const char *, ...) {
    return 1;
}

int main(void) {
    return log("hello", 1, 2, 3, 4, 5, 6, 7, 8, 9);     //expect 1
}
```

## Invalid Tests
```C
int f(int x, ...) {
    return x;
}

int main(void) {
    f();      // invalid
}
```

```C
int sum(int a, int b, ...) {
    return a + b;
}

int main(void) {
    return sum(1);     // invalid
}
```
