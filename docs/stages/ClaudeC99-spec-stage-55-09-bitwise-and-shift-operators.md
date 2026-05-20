# ClaudeC99 55-09 bitwise and shift operator

## Task
  - Support bitwise and shift preprocessor operators

## Support
``` ~ & ^ | >> <<```

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