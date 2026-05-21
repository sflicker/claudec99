# ClaudeC99 stage 57-01 function like macro stringification

## Task
  - Add support for macro stringification using `#` inside function-like macro replacement lists.

Example:
```C
#define STR(x) #x

int strcmp(const char *, const char *);

int main(void) {
    return strcmp(STR(hello), "hello");  // expect 0
}
```

In this example `STR(hello)` expands to `hello`

## Rules
  - `#` is only special inside a function-like macro replacement list
  - `#` must be followed by a macro parameter name
  - The actual argument tokens are converted into a string literal
  - The argument is not macro-expanded before stringification
  - Whitespace inside the argument should be normalized to a single space
  - Quotes and backslashes in the stringified result should be escaped
  - existing object-like macros and function-like macros behavior mush continue working

## Out of scope
  - token pasting ##
  - Variadic macros
  - full macro rescan hardening beyond what already exists
  - two-level expansion tricks beyond what naturally works from existing macro expansion
  - exact GCC/Clang whitespace behavior in every edge case

## Basic Behavior
Simple Identifier

```C
#define STR(x) #x

int strcmp(const char *, const char *);

int main(void) {
    return strcmp(STR(coffee), "coffee");  // expect 0
}
```

Expression text
```C
#define STR(x) #x
int strcmp(const char *, const char *);

int main(void) {
    return strcmp(STR(1 + 2), "1 + 2");  // expect 0
}
```
The expression is not evaluated it is turned into a string

Whitespace trimmed to one space
```C
#define STR(x) #x
int strcmp(const char *, const char *);

int main(void) {
    return strcmp(STR(1     + 2), "1 + 2");  // expect 0
}
```


Macro argument is not expanded before stringification
```C
#define NAME Bob
#define STR(x) #x
int strcmp(const char *, const char *);

int main(void) {
    return strcmp(STR(NAME), "NAME");   // expect 0.   
}
```
STR(NAME) should become "NAME", not "Bob"

Escaping Tests
String Argument
```C
#define STR(x) #x
int strcmp(const char *, const char*);

int main(void) {
    return strcmp(STR("HELLO"), "\"HELLO\"");   // expect 0
}
```

## Tests - Use the examples above as basis for valid tests

## Invalid Tests
Not followed by parameter
```C
#define BAD(x) #y      // INVALID expact error
    
int main(void) {
    return 0;
} 
```

Bare # in replacement list
```C
#define BAD(x) #        // INVALID expect error

int main(void) {
    return 0;
}
```

Stringification in object-like macro
```C
#define BAD #x      // INVALID expect error

int main(void) {
    return 0;
}
```