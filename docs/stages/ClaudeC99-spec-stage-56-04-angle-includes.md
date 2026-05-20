# ClaudeC99 stage 56-04 angle includes

## Task
  - add support for angle includes

## In-scope
  - Parse `#include <name.h>`
  - Search only in -I directories
  - Support multiple -I directories in command line order

## Search Order
** For quoted includes **
  - Directory of the including file
  - -I directories in order
** For Angle includes **
  - -I directories in order

## Out of scope
  - host system include directories like /usr/include
  - compiler default include directories
  - -isystem
  - standard header compatibility
  - #include_next

## Test
```C
------ include/add.h ------
int add(int a, int b);
---------------------------

-------- add.c ------------
#include <add.h>  

int add(int a, int b) {
    return a + b;
}
---------------------------

-------- test main --------
#include <add.h>

int main() {
    return add(3,4);   // expect 7
}
--------------------------

----- .cflags ------------
-Iinclude
--------------------------
```