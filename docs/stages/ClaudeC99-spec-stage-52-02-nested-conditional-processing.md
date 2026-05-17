# ClaudeC99 Stage 52-02 nested conditional processing

## Task
  - Extend conditional processing so `#ifdef`, `#ifndef`, `#else` and `#endif` work correctly when nested.

## Tests
Nested true/true

```C
#define OUTER
#define INNER

#ifdef OUTER
#ifdef INNER
int main() {
    return 42;    // expected status code: 42
}
#else
int main() {
    return 1;
}
#endif
#else
int main() {
    return 2;
}
#endif
```

Nested true/false

```C
#define OUTER

#ifdef OUTER
#ifdef INNER
int main() {
    return 42;    
}
#else
int main() {
    return 1;     //expected status code: 1
}
#endif
#else
int main() {
    return 2;
}
#endif
```

Nested false/true

```C
#define INNER

#ifdef OUTER
#ifdef INNER
int main() {
    return 42;    
}
#else
int main() {
    return 1;
}
#endif
#else
int main() {
    return 2;    // expected status code: 2
}
#endif
```

** Include Guard patter with duplicate include**
```C
----- helper.h ---
#ifndef HELPER_H
#define HELPER_H
#define VALUE 42
#endif
------------------

----- main .c test file --
#include "helper.h"
#include "helper.h"

int main() {
    return VALUE;    // expect status code 42
}
-------------------------
```

## Invalid tests
missing endif
```C
#define OUTER

#ifdef OUTER
#ifdef INNER
int main() {
   return 1;
}
#endif
// INVALID MISSING #endif
```

inner duplicate else
```C
#define OUTER

#ifdef OUTER
#ifdef INNER
int main() {
    return 1;
}
#else
int main() {
    return 2;
}
#else      // INVALID DUPLICATE #else
int main() {
    return 3;
}
#endif
#endif
```
