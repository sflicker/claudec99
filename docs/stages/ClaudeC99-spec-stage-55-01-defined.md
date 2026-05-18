# ClaudeC99 Stage 55-01  `defined` in `#if` and `elif`

## Task
  - add support for the `defined` operator in preprocessor conditional expressions.
  - support `#if defined(NAME)`
  - support `#elif defined(NAME)`

## Rules
  - defined(NAME) evaluates to 1 if NAME is currently defined as a macro
  - defined(NAME) evaluates to 0 if NAME is not currently defined as a macro

## Out of Scope
  - macro expansion in #if expressions
  - &&, ||, !
  - comparison operators
  - arithmetic expressions
  - treating arbitrary identifiers as 0

## Tests
```C
#define DEBUG

#if defined(DEBUG)
int main() { return 42; }   // expect status code 42
#else
int main() { return 1; }
#endif
```

```C
#define DEBUG
#undef DEBUG

#if defined(DEBUG)
int main() { return 42; }  
#else
int main() { return 1; } // expect status code 1
#endif
```

```C
#define FIRST

#if defined(SECOND)
int main() { return 1; }
#elif defined(FIRST)
int main() { return 42; }  // expect status code 42
#else
int main() { return 2; }
#endif
```