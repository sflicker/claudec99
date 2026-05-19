# ClaudeC99 stage 55-06 equality and relational operators in `#if` and `#elif`

## Task
  - Add binary comparison operators to preprocessor conditional expressions

## In-Scope

**Support**
```C
#if VALUE == 1
#if VALUE != 1
#if VALUE < 10
#if VALUE <= 10
#if VALUE > 0
#if VALUE >= 1
#elif VALUE == 1
#elif VALUE != 1
#elif VALUE < 10
#elif VALUE <= 10
#elif VALUE > 0
#elif VALUE >= 1
```

**Example**
```C
#define VERSION 3

#if VERSION >= 2
int main() { return 42; }   // expect 42
#else
int main() { return 1; }
#endif
```

## Operators
Add;
```C
==
!=
<
<=
>
>=
```

## Out of scope
  - &&
  - ||
  - arithmetic operators
  - bitwise operators
  - shift operators


## Tests

```C
#define VERSION 3

#if VERSION >= 2
int main() { return 42; }   // expect 42
#else
int main() { return 1; }
#endif
```

```C
#define VALUE 0

#if VALUE != 0
int main() { return 1; }
#else
int main() { return 42; }   // expect 42
#endif
```

```C
#define VERSION 6

#if VERSION > 8
int main() { return 1; }
#elif VERSION >= 6
int main() { return 42; } // expect 42
#else
int main() { return 2; }
#endif
```