# ClaudeC99 Stage 52-03 Basic `#if` constant conditions

## Task
  Add limited `#if` support for simple integer constants
  
## In Scope
```C
#if 1
...
#endif
```

```C
#if 0
...
#else
...
#endif
```

Allow whitespace
```C
#if     1
```

Allow simple integer values
```C
#if 42
```

## Rules
For this stage, `#if` must be followed by optional whitespace and then a single 
integer literal. Whitespace may appear between `#` and `if`, and between `if` 
and the integer literal. Forms like `#if0` are not valid directives.
Parenthesized expressions such as `#if(1)` are valid C but out of scope for this stage.
```C
0 = false
nonzero = true

```

## Out of scope
  - `#elif`
  - `defined(NAME)`
  - macro expansion inside `#if`
  - arithmetic expressions
  ` comparisons expressions
  - logical expressions

## Valid Tests
```c
#if 1
int main() {
    return 42;   // expect status code 42
}
#else
int main() {
    return 1;
}
#endif
```

```c
#if 0
int main() {
    return 42;   
}
#else
int main() {
    return 1;   // expect status code 1
}
#endif
```

```c
#if 113
int main() {
    return 42;   // expect status code 42
}
#else
int main() {
    return 1;
}
#endif
```

## invalid tests
```C
#if        // INVALID 
int main() {
    return 42;
}
```

```C
#if X     // INVALID
int main() {
    return 1;
}
```

```C
#if 1 + 4   // INVALID
int main() {
    return 3;
}
```