# ClaudeC99 Stage 57-04 Variadic macros

## Task
Add support for function like macros with `...` and `__VA_ARGS__`

## Example 
```C
#define LOG(fmt, ...) printf(fmt, __VA_ARGS__)
```

Then
```C
LOG(%d\n", 42);
```

Expands to 
```C
printf("%d\n", 42);
```

## In-Scope
  - parse vaiadic macro definitions:
  - ```C
  - #define M(...)
  - #define M(x, ...)
  - ```
  - support __VA_ARGS__ in the replacement list
  - substitute all extra arguments as comma-separated token sequence
  - validate required fixed arguments
  - reject missing variadic arguments

## Out of Scope
  - GNU args... named variadic macros
  - GNU comma deletion with , ## __VA_ARGS__
  - __VA_OPT__
  - perfect GCC/CLANG compatibility

## Test
----- .expected --------
hello
------------------------

------ main .f ---------
```C
int printf(const char *, ...);
#define LOG(fmt, ...) printf(fmt, __VA_ARGS__)

int main(void) {
    LOG("%s\n", "hello")
    return 0;   //expect 0
}
```
