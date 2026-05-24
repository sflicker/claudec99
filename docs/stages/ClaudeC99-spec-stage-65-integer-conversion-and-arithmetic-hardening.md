# ClaudeC99 Stage 65 integer conversion and arithmetic hardening

## Task
  - add additional tests to validate integer conversion and arithmetic function properly
    with the new `_Bool` and `long long` types along with the updates to `signed` and `unsigned`.
  - These new tests should use the project/controlled versions (in <project_root>/test/include) of the standard includes where appropriate.

## Tests
_Bool promotes to int
```C
#include <stdbool.h>

int main(void) {
    bool b;
    int x;
    
    b = 99;
    x = b + 41;
    
    return x == 42;   // expect 1
}
```

`signed char` promotes preserving sign
```C
#include <stdint.h>

int main(void) {
    int8_t c;
    int x;
    
    c = -1;
    x = c + 2;
    
    return x == 1;     // expect 1
}
```

`unsigned char` promotes to `int`, not necessarily `unsigned int`

```C
#include <stdint.h>

int main(void) {
    uint8_t c;
    int x;
    
    c = 255U;
    x = c + 1;
    
    return x == 256;   //expect 1
}
```

`short` promotes preserving sign
```C
#include <stdint.h>

int main(void) {
    int16_t s;
    int x;
    
    s = -2;
    x = s + 5;
    
    return x == 3;       // expect 1
}
```

`unsigned short` promotes to `int`
```C
#include <stdint.h>

int main(void) {
    uint16_t s;
    int x;
    
    s = 65535U;
    x = s + 1;
    
    return x == 65536;    // expect 1
}
```

`signed/unsigned` int comparison
```C
int main(void) {
    int s;
    unsigned int u;
    
    s = -1;
    u = 1U;
    
    return s < u;     //expect 0
}
```

```C
int main(void) {
    signed s;
    unsigned u;
    
    s = -1;
    u = 1U;
    
    return s < u;       // expect 0 
}
```

```C
int main(void) {
    return -1 < 1U;       // expect 0
}
```

signed long vs unsigned int on LP64
```C
int main(void) {
    long s;
    unsigned int u;
    
    s = -1L;
    u = 1U;
    
    return s < u;     // expect 1
}
```
NOTE: since long can represent every unsigned. THe unsigned int converts to long, not the other way around

signed long vs unsigned long
```C
int main(void) {
    long s;
    unsigned long u;
    
    s = -1L;
    u = 1UL;
    
    return s < u;     // expect 0
}
```
NOTE: s converts to unsigned long, so -1L becomes a very large unsigned value

`long long` dominates `int`
```C
int main(void) {
    long long x;
    int y;
    
    x = 40LL;
    y = 2;
    
    return x + y == 42LL;    // expect 1;
}
```

`unsigned long long` dominates signed types
```C
int main(void) {
    unsigned long long u;
    long long s;
    
    u = 1ULL;
    s = -1L;
    
    return s < u;     // expect 0
}
```
NOTE: s converts to `unsigned long long`

Assignment conversion to smalled unsigned type
```C
#include <stdint.h>

int main(void) {
    uint8_t x;
    
    x = 257U;       // 257U should trunctate to 1 since x is unsigned char;
    
    return x == 1;    // expect 1
}
```

assignment conversion to _Bool
```C
#include <stdbool.h>

int main() {
    bool b;
    int x;
    int y;
    
    x = 3;
    y = -3;
    
    b = x + y;
    if (b != false) {
        return 0;
    }
    
    b = x + 1;
    return b != false;     // expect 1
}
```

Conditional expression mixed integer types
```C
int main(void) {
    unsigned long x;
    int cond;
    
    cond = 1;
    x = cond ? 42U : 9L;
    
    return x == 42UL;   //expect 1
}
```

Compound assignment conversion
```C
#include <stdint.h>

int main(void) {
    uint8_t x;
    
    x = 250U;
    x += 10U;
    
    return x == 4U;    // expect 1;
}
```

Usual arithmetic conversion with multiplication
```C
int main(void) {
    unsigned long long x;
    int y;
    
    x = 1000000LL;
    y = -1;
    
    return x * y > 0ULL;      // expect 1;
}
```
NOTE: y converts to unsigned long long producing a huge unsigned value before multiplication
