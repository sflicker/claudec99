# ClaudeC99 55-09 bitwise and shift operator

## Task
  - Support bitwise and shift preprocessor operators

## Support
``` ~ & ^ | >> <<```

## operator precedence
Use the same precedence as counterparts in c
Here is the full order with order 
```C
primary / parenthesized / defined / literals identifiers-as-0
unary   ! + - ~
multiplicative * / %
additive + -
shift >> <<
relational
< <= > >=
equality == !=
bitwise AND   &
bitwise XOR   ^
bitwise OR    |
logical AND   &&
logical OR    ||
```

## Tests
```C
#define VALUE 1

#if ~VALUE
int main() { return 42; }
#else
int main() { return 1; }  // expect 1
#endif
```

```C
#define A 1
#define B 1

#if A & B
int main() { return 42; } // expect 42
#else
int main() { return 1; }
#endif
```

```C
#define A 1
#define B 0

#if A ^ B
int main() { return 42; } // expect 42
#else
int main() { return 1; } 
#endif 
```

```C
#define A 1
#define B 1

#if A ^ B
int main() { return 42; } 
#else
int main() { return 1; } // expect 1
#endif 
```

```C
#define A 1
#define B 0

#if A | B
int main() { return 42; } // expect 42
#else
int main() { return 1; }
#endif
```

```C
#define A 1

#if A << 1
int main() { return 42; } // expect 42
#else
int main() { return 1; }
#endif
```

```C
#define A 1

#if A >> 1
int main() { return 42; }
#else
int main() { return 1; } // expect 1
#endif
```

```C
#if 1 + 2 << 3
int main() { return 42; }   // expect 42
#else
int main() { return 1; }
```

```C
#if 16 >> 1 >> 2
int main() { return 42; }
#else
int main() { return 1; }
#endif
```

```C
#if (6 & 3) == 2
int main() { return 42; }
#else
int main() { return 1; }
#endif
```
