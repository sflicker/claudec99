# ClaudeC99 Stage 56-03 Command-Line include flags `-I`

## Task
  - Add support for compiler include search paths using `-I`

## In-scope
Support
```Sh
ccompiler -Itest/include main.c 
ccompiler -I test/include main.c
```

Both forms are allowed. Also multiple -I options can be used
For this stage, apply -I search paths to quoted includes.

-I paths are interpreted relative to the current working directory where
ccompiler is involved, unless they are absolute paths.

```C
#include "helper.h"
```

## Search order
for quoted includes, use:
1. the directory of the including file
2. directories provided with -I in command-line order

## Out of Scope
  - angle includes: #include <stdio.h>
  - System include directories
  - Environment include paths
  - -isystem
  - Header search diagnostics beyond a clean "include file not found" message

## Tests
```C
----- include/math.h -----
#ifndef MATH_H_
#define MATH_H_
int add(int a, int b);
#endif
----------------------------

---- settings/include/settings.h -
#ifndef SETTINGS_H_
#define SETTINGS_H_
#define FUNC 1
#endif
----------------------------

-------- math.c ----------
#include "math.h" 
int add(int a, int b) {
    return a + b;
}
----------------------------

---- .cflags ---------------
-Iinclude -Isettings/include -DA=3 -DB=4
----------------------------

-------- main test .c ------
#include "math.h"
#include "settings.h"

int main() {
#if FUNC == 1
    return add(A,B);    // expect 7
#else
    return 0;
#endif
}
----------------------------
```