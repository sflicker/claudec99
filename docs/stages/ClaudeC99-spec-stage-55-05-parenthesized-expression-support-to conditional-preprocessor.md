# Claude 55-05 Parenthesized `#if` and `#elif` expressions

## Task
  - Add parenthesized expression support to preprocessor conditional expressions

## In-Scope
  - `(expr)`
  - nested parentheses
  - parenthesized integer literals
  - parenthesized identifiers/macros
  - parenthesized `defined(...)`
  - combinations with unary operators

## examples
```C
#define FLAG 1

#if (FLAG)
int main() { return 42; }   // expect 42
#else
ing main() { return 1; }
#endif
```

```C
#if !defined(DEBUG)
int main() { return 42; }
#else
int main() { return 0; }
#endif
```

#Out of Scope

- binary arithmetic
- comparisons
- &&
- ||

## Tests

```C
#define FLAG 1

#if (FLAG)
int main() { return 42; }   // expect 42
#else
ing main() { return 1; }
#endif
```

```C
#if !defined(DEBUG)
int main() { return 42; }
#else
int main() { return 0; }
#endif
```

```C
#if (1)
int main() { return 42; } // expect 42
else
int main() { return 1; }
```

```C
#if (0)
int main() { return 1; }
#elif (1)
int main() { return 42; } // expect 42
#else
int main() { return 2; }
#endif
```

```C
#define VALUE 1

#if (!VALUE)
int main() { return 1; }
#elif (VALUE)
int main() { return 42; }  // expect 42
#else
int main() { return 2; }
#endif
```

```C
#define VALUE 1

#if (-VALUE)
int main() { return 1; }  // expect 1
#elif (VALUE)
int main() { return 42; }  
#else
int main() { return 2; }
#endif
```

```C
#if (((1)))
int main() { return 42; } // expect 42
#else
int main() { return 1; } 
#endif
```

```C
#define DEBUG 1
#if (defined(DEBUG))
int main() { return 42; } // expect 42
#else
int main() { return 1; }
#endif
```