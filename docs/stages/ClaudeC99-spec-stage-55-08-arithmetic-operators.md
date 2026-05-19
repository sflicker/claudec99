# ClaudeC99 - 55-08 arithmetic operators

## Task
  - Add basic binary operator support to preprocessor conditional expressions

## In Scope
```C
#if A + B
#if A - B
#if A * B
#if A / D
#if A % B
```

## Normal C-like precedence
```C
unary/primary
* / %
+ -
< <= > >=
== !=
&&
||
```

## Error cases
  - Division by zero
  - Module by zero
  - malformed binary expressions

## Out of Scope
  - bitwise operators
  - shift operators
  - ternary `?:`
  - sizeof
  - character constants in preprocessor expressions

## Tests
```C
#define A 20
#define B 22

#if A + b == 42
int main() { return 42; } // expected 42
#else
int main() { return 1; }
#endif
```

```C
#define X 10

#if X % 2
int main() { return 42; }
#else
int main() { return 1; }  // expected 1
#endif
```

```C
#define X 10

#if X % 2 == 0
int main() { return 42; } // expected 42
#else
int main() { return 1; }
#endif
```

```C
#define A 10
#define B 4
#define C 2

#if C + A * B == 42
int main() { return 42; } // expected 42
#else
int main() { return 1; }
#endif
```