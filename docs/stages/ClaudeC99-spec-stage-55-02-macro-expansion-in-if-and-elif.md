# ClaudeC99 Stage 55-02 macro expansion in `#if` and `elif`

## Task 
  - Allow object-like macros to be expanded when evaluating `#if` and `#elif` expressions
  - support the `#if defined NAME` syntax that tests if the macro NAME is defined.
    - Similar to #if defined(NAME) from stage 52-01
    

## In scope
  - support simple object like macros that expand to integer literals
  
```C

```

## Requirements
For this stage An `#if` or `elif` may be one of (examples apply for `#elif` as well)
  - an integer literal
    - `#if 1`

  - a defined operator expression
    - `#if defined(NAME)`
    - `#if defined NAME`
    
  - an object-like macro that expands to a single integer literal
    - `#define ENABLED 1`
    - `#if ENABLED`

## out of scope
  - function-like macro expansion inside `#if`
  - arithmetic expressions
  - comparison operators
  - logical operators
  - undefined identifiers becoming 0
  - recursive macro expression evaluation beyond simple object like macros

## Tests
```C
#define DEBUG 1
#if DEBUG
int main() { return 42; } // expected status code 42
#else
int main() { return 1; }
#endif
```

```C
#define DEBUG 0
#if defined DEBUG
int main() { return 42; } // expected status code 42
#else
int main() { return 1; }
#endif

```

```C
#define RED 0
#define GREEN 0
#define BLUE 1
#if RED
int main() { return 0; }
#elif GREEN 
int main() { return 1; }
#elif BLUE
int main() { return 42; } // expected stagus code 42
#endif
```

```C
#define RED 0
#define GREEN 0
#define BLUE 1
#if defined RED
int main() { return 0; } // expected status code 0
#elif defined GREEN 
int main() { return 1; }
#elif defined BLUE
int main() { return 42; } 
#endif
```

```C
#define RED 0
#define GREEN 0
#define BLUE 1
#if defined(RED)
int main() { return 0; } // expected status code 0
#elif defined(GREEN) 
int main() { return 1; }
#elif defined(BLUE)
int main() { return 42; } 
#endif
```
