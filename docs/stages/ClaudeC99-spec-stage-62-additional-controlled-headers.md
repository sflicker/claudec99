# ClaudeC99 Stage 61 - additional controlled headers (limits.h and stdint.h)

## Task
  - add additional controlled standard headers to the <project_root>/test/include directory

```C
------ limits.h -------
#ifndef CLAUDEC99_LIMITS_H
#define CLAUDEC99_LIMITS_H

#define CHAR_BIT __CHAR_BIT__

#define SCHAR_MIN (-128)
#define SCAR_MAX 127
#define UCHAR_MAX 255

#define SHRT_MIN (-32868)
#define SHRT_MAX 32767
#define USHRT_MAX 65535

#define INT_MIN (-2147483647 -1)
#define INT_MAX 2147483647

#define LONG_MIN (-9223372036854775807L - 1L)
#define LONG_MAX 9223372036854775807L

#endif
--------------------------
```

```C
-------- stdint.h --------
#ifndef CLAUDEC99_STDINT_H
#define CLAUDEC99_STDINT_H

/* exact-width signed integer types */
typedef signed char int8_t;
typedef short int16_t;
typedef int int32_t;
typedef long int64_t;

/* exact width unsigned integer types */
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned ln09g uint64_t;

/* pointer-size integer types */
typedef long intptr_t;
typedef unsigned long uintptr_t;

/* minimum width integer types */
typedef int8_t int_least8_t;
typedef int16_t int_least16_t;
typedef int32_t int_least32_t;
typedef int64_t int_least64_t;

typedef uint8_t uint_least8_t;
typedef uint16_t uint_least16_t;
typedef uint32_t uint_least32_t;
typedef uint64_t uint_least64_t;

/* fast integer types - simple LP64 choices */
typedef int int_fast8_t;
typedef int int_fast16_t;
typedef int int_fast32_t;
typedef long int_fast64_t;

typedef unsigned int uint_fast8_t;
typedef unsigned int uint_fast16_t;
typedef unsigned int uint_fast32_t;
typedef unsigned long uint_fast64_t;

/* greatest-width integer types available for now */
typedef long intmax_t;
typedef unsigned long uintmax_t;

#endif


## tests
**These tests should work as *Valid* tests**


```C
#include <stdint.h>

int main(void) {
    int8_t a;
    uint8_t b;
    int16_t c;
    uint16_t d;
    int32_t e;
    uint32_t f;
    int64_t g;
    uint64_t h;
    intptr_t p;
    uintptr_t q;

    a = -1;
    b = 255;
    c = -2;
    d = 65535;
    e = -3;
    f = 42;
    g = -4;
    h = 99;
    p = -5;
    q = 100;

    return sizeof(a)    // 1
        +  sizeof(b)    // 1 + 1 → 2
        +  sizeof(c)    // 2 + 2 → 4
        +  sizeof(d)    // 4 + 2 → 6
        +  sizeof(e)    // 6 + 4 → 10
        +  sizeof(f)    // 10 + 4 → 14
        +  sizeof(g)    // 14 + 8 → 22
        +  sizeof(h)    // 22 + 8 → 30
        +  sizeof(p)    // 30 + 8 → 38
        +  sizeof(q);   // 38 + 8 → 46         [EXPECT 46]
}

```C
#include <stdint.h>
#include <stddef.h>

int main(void) {
    int32_t x;
    uintptr_t p;
    
    x = 42;
    p = (uintptr_t)&x;
    
    return p != NULL;
```