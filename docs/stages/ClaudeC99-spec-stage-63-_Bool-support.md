# ClaudeC99 Stage 63 - _Bool support and add stdbool.h controlled header

## Task
  - add support for the _Bool type
  - add stdbool.h to the controlled headers in <project.root>/test/include
  - update the <project.root>/test/include/limits.h to include additional defines

## Token Updatea
add a keyword token for `_Bool`

## Grammar Update
```ebnf
<type_specifier> ::= "void"
                     | <integer_type> 
                     | "_Bool"
                     | <typedef_name> 
                     | <enum_specifier> 
                     | <struct_specifier>
```

## Notes
  - _Bool should have size of 1 byte
  - _Bool should have alignment of 1 bytes
  - Reject Invalid combinations
    - unsigned _Bool x;    // invalid
    - signed _Bool x;      // invalid
    - short _Bool x;       // invalid
    - long _Bool x;        // invalid
  - _Bool should be a real type
  - _Bool should be a scalar type
  - _Bool should be integer likecan 
  - key semantic rule
    - any value converted to _Bool becomes
    - `0 → 0`
    - `nonzero → 1
    ```C
        _Bool a ;
        a = 0;  // stores 0
        a = 42; // stores 1
        a = -7; // stores 1 
    ```
  - _Bool can be used in expressions with other integer types and normal conversion rules apply.
      ```C
      _Bool a = 1;
      int b = 2;
      int c = a + b;      // a is promoted to int for the `+` expression. so c would be 3.
      ```
  - _Bool special rule effects expressions. 
  - ```C
        _Bool a = 1;
        int b = 3;
        int c = 4;
        a = a + b + c;       // however since all nonzero values collapse to 1 result of a after the assignment would be 1. 
  - ```

what 
Add new stdbool.h to <project.dir>/test/include directory
```C
--------- stdbool.h ------
#ifndef CLAUDEC99_STDBOOL_H
#define CLAUDEC99_STDBOOL_H

#define bool _Bool
#define true 1
#define false 0
#define __Bool_true_false_are_defined 1
--------------------------
```

```C
additional defines for limits.h. current entries should remain
---------- limits.h -----------
#define UINT_MAX 4294967295U
#define ULONG_MAX 18446744073709551615UL
```

## Tests
```C
int main(void) {
    _Bool a;
    int b;
    int c;
    
    a = 0;
    b = 3;
    c = 4;
    
    a = b + c;     // expect 1
}
```

```C
int main(void) {
    _Bool a;
    int b;
    int c;

    b = 3;
    c = -3;

    a = b + c;

    return a;    // expect 0
}
```

```C
int main(void) {
    _Bool a;
    int b;
    int c;
    
    a = 100;
    b = 2;
    c = a + b;      // expect 3
}
```

```C
int main(void) {
    _Bool a;
    a = 42;
    return a + 41;    // expect 42
}
```

_Bool should work with the defines in stdbool.h
```C
#include <stdbool.h>

int main() {
    bool a = true;
    bool b = false;
    bool c = a | b;
    return c;     // expect 1
}
```

Tests for limits.h changes
```C
#include <limits.h>

int main() {
#ifdef UINT_MAX
    return 1;     // expect 1
#else
    return 0;
#endif
}
```

```C
#include <limits.h>

int main() {
#ifdef ULONG_MAX
    return 1;     // expect 1
#else
    return 0;
#endif
}
```

```C
#include <limits.h>
#include <stdbool.h>

int main() {
    bool a = UINIT_MAX < ULONG_MAX;
    return a;     // expect 1
}
```