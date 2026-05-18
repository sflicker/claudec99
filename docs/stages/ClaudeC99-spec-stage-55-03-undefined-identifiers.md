# ClaudeC99 55-03 Undefined identifiers in `#if` and `#elif` evaluate to 0

## Task 
  - Update proprocessor conditional expression handling so that bare undefined identifiers in `#if` and `#elif` evaluate to 0.

## In Scope
Support
```C
#if UNDEFINED_NAME
...
#else
...
#end
```
as false

Support
```C
#define ENABLED 1
#if ENABLED
...
#endif
```
as true

Support
```C
#define DISABLED 0
#if DISABLED
...
#else
...
#endif
```
as false

**Rule**
FOR THIS STATE:
For `#if NAME`

if NAME is an object like macro expanding to a single integer literal:
   use that integer value
else if NAME is undefined:
   use 0
else
    report unsupported #if expression

## Out of Scope
  - arithmetic expression
  - comparison operators
  - logical operators
  - parenthesized expressions
  - function-like macro expansion in `#if`
  - full recursive macro expansion

## Tests
```C
#if MISSING
int main() { return 1; }
#else
int main() { return 42; } // expected status code: 42
```

```C
#define FLAG 0

#if FLAG
int main() { return 1; }
#else
int main() { return 42; }   // expected status code: 42
#endif
```

```C
#define FLAG 1
#undef FLAG

#if FLAG
int main() { return 1; }
#else
int main() { return 42; }   // expected status code: 42
#endif
```