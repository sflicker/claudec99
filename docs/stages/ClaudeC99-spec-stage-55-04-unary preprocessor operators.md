# ClaudeC99 55-04 Unary preprocessor operators

## Task
  - Add basic unary operator support to preprocessor conditional expressions.

## In Scope

Support:
```C
#if !FLAG
#endif

#if -1
#endif

#if +1
#endif
```

## Rules For This Stage
  - Evaluate unary +, unary -, and ! on logical literal/macro-expanded integer values.
  - Then apply the normal #if rule: zero is false, non-zero is true
```C
#if -1     true
#if +1     true
#if -0     false
#if !0     true
#if !1     false
#if !-1    false  
```

## Out of Scope
  - binary arithmetic
  - comparison operators
  - logical && / ||
  - parentheses
  - function-like macro expansion inside `#if`

## Tests
```C
# define FLAG 0

#if !FLAG
int main() { return 42; } // expect status code 42
#else
int main() { return 1; }
#endif
```

```C
# define FLAG 0

#if -FLAG
int main() { return 42; } 
#else
int main() { return 1; }  // expect status code 1
#endif
```

```
# define FLAG 1

#if !FLAG
int main() { return 42; } 
#else
int main() { return 1; }  // expect status code 1
#endif
```

```
# define FLAG 1

#if -FLAG
int main() { return 42; } // expect status code 1
#else
int main() { return 1; } 
#endif
```

```
# define FLAG 1

#if +FLAG
int main() { return 42; } // expect status code 1
#else
int main() { return 1; } 
#endif
```

```C
#if -1
int main() { return 42; } // expect status code 42
#else
int main() { return 1; }
do #endif
```

```C
#if +1
int main() { return 42; } // expect status code 42
#else
int main() { return 1; }
do #endif
```

```C
#if -0
int main() { return 42; } 
#else
int main() { return 1; }   //expect status code 1
do #endif
```

```C
#if !0
int main() { return 42; } // expect status code 42
#else
int main() { return 1; }
do #endif
```

```C
#if !1
int main() { return 42; } 
#else
int main() { return 1; }   //expect status code 1
do #endif
```

```C
#if !-1
int main() { return 42; } 
#else
int main() { return 1; }   //expect status code 1
do #endif
```

