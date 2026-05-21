# ClaudeC99 57-03 Variadic function calls and declarations

## Task
Add limited support for declaring and calling variadic functions.

## In-scope
Support function declarations like:

```C
int printf(const char *, ...);

printf("hello\n");
printf("%d\n", 42);
```

## Rules
  - `...` may appear only at the end of a parameter list.
  - A variadic function must have at least one named/fixed parameter.
  - Calls must provide at least the fixed parameter count.
  - Extra arguments are allowed.
  - Type-check fixed parameters normally
  - Type-check extra arguments only enough to compile them
  - Support calling external libc variadic functions like printf

## Out of Scope
  - Defining variadic functions
    - `int f(const char *fmt, ...) {...}`
    - va_list
    - va_start
    - va_arg
    - va_end
    - <stdarg.h>
    - floating-point variadic calling rules

## For x86-64 System V, there is one important codegen detail:
   For calls to variadic functions, `%al` should contain the number of
   vector registers used for floating-point arguments. Since floating-point
   arguments are not supported by this project yet, set that register
   to zero before the call
   Example
   ```asm
     xor eax, eax      ;
     call printf
   ```

## test

```C
------ .expected --------
hello
-------------------------

------- main test .c ----
int printf(const char *, ...);

int main() {
    printf("hello\n");
    return 0;        // expect 0
}
--------------------------
```

```C
------ .expected --------
42
-------------------------

------- main test .c ----
int printf(const char *, ...);

int main() {
    printf("%d\n", 42);
    return 0;          // expect 0
}
--------------------------
```