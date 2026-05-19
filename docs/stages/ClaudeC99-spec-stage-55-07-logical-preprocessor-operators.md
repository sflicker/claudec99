# ClaudeC99 55-07 logical preprocessor operators

## Task
  - Add logical `&&` and `||` support to preprocessor conditional expressions

## In-Scope
Support
```C
#if A && B
#if A || B
#if defined(A) && A
#if defined(A) || defined(B)
#if VERSION >= 2 && ENABLED
```

**Use normal truthiness**
0 = false
nonzero = true

Result of && and || should be
0 = false
1 = true

**Use c-like precedence
```C
unary/primary
< <= > >=
== !=
&&
||
```

so `#if A || B && C`  =>  `#if A || (B && C)`

## Out of Scope
  - arithmetic binary operators
  - bitwise operators
  - shifts
  - ternary `?:`

## Tests
```C
#define A 1
#define B 0
#define C 1

#if A && C
int main() { return 42; }  // expect 42
#else 
int main() { return 1; }
#endif
```

```C
#define A 0
#define B 1
#define C 0

#if A || B && C
int main() { return 1; }
#else
int main() { return 42; } // expect 42
#endif
```

```C
#define PRIORITY 1
#define SERVERITY 2

#if PRIORITY == 1 && SEVERITY == 1
int main() { return 1; }
#elif PRIORITY == 1 && SEVERITY == 2
int main() { return 42; } // expect 42
#else
int main() { return 2; }
#endif


```

