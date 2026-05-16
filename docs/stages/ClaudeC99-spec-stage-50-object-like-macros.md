# ClaudeC99 stage 50 - object like macros

## Task
   Add support for simple object-like `#define` macros

Example
```C
#define ANSWER 42

int main() {
    return ANSWER;   // EXPECT status code 42.
}
```

## In Scope
  - parse `#define NAME replacement`
  - store object-like macro definitions
  - expand macro names in ordinary source text
  - support empty replacement macros
  - support replacement lists with identifiers, literals, operators and punctuation
  - allow macros to be used across included headers
  - reject duplicate incompatible macro definitions cleanly

## Out of scope
  - function like macros
  - macro arguments
  - stringification #
  - tool pasting ##
  - conditional preprocessing
  - recursive macro expansion beyond simple guarded expansion
  - predefined macros like __FILE__ and __LINE__

## Tests

Single file usage
```C
#define SIZE 10
#define VALUE 42

int main() {
    int a[SIZE];
    a[0] = VALUE;
    return a[0];    // EXPECT STATUS CODE: 42
}
```

Multi file usage
```C
--- constants.h ---
#define ANSWER 42
-------------------

--- main test .c file ---
#include "constants.h"

int main() {
    return ANSWER; // EXPECT status code: 42
}
```

Simple expression
```C
--- main test .c file
#define SUM 1+2+3+4

int main() {
    return SUM;    // expect 10
}
```

duplicate compatible
```C
#define ANSWER 42
#define ANSWER 42      // this should work

int main() {
    return ANSWER;  // expect status code 42
}
```

duplicate incompatible
```C
#define ANSWER 42
#define ANSWER 43    // this should be invalid

int main () {
   return 0;
}
```

more defines - expect status code 2
```C
#define INIT int a=1;
#define INC a++;
#define RETURN return a;

int main() {
   INIT
   INC
   RETURN
}
```