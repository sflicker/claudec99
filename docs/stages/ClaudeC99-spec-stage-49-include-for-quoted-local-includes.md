# Claude Stage 49 - quoted local includes

## Task
  - add support for local quoted includes

example
```C
  #include 'helper.h" 
  .. rest of file
```

## In Scope
  - recognize `#include "file.h"`
  - search relative to the including file's directory
  - support nested includes
  - preserve stage 48 behavior
    - line splicing
    - comment removal
    - unsupported directive rejection
  - detect and report missing include file cleaning
  - detect obvious include recursion or impose a maximum include depth

## Out of Scope
  - angle includes: `include <stdio.h>`
  - include search paths like `-I`
  - include guards
  - `#pragma once`
  - macros
  - conditional processing
  - system headers

## Tests
### Test harness update
   for integration tests support a <basename>.error - if present the harness should check
   for an error message during compiling. 
### Integration Tests
base local include
```C
--- add.h ---
int add(int a, int b);
-------------

--- add.c ---
#include "add.h";

int add(int a, int b) {
    return a + b;
}
--------------

--- main test file ---
#include "add.h"

int main() {
    return add(3,4);   // expect exit code 7
}
------------
```

nested include
```C
--- multi.h ---
int multi(int a, int b);
---------------

--- helper.h ---
#include "multi.h"
----------------

--- multi.c ---
#include "multi.h"
int multi(int a, int b) {
    return a * b;
}
---------------

--- Main Test .c file ---
#include "helper.h"

int main() {
    return multi(3,4);   // expect exit code 12
}
-----
```

Recursive Include
This test should be in integration tests with an <basename>.error

```C
--- helper.h ---
#include "helper.h"
----------------

--- main test .c file ---
#include "helper.h"

int main() {
   return 0;
}
```

Missing include file
This test should be in integration tests with an <basename>.error
```C
--- main test .c file ---
#include "helper.h"     // this should trigger an error
int main() {
    return 0;
}
```
