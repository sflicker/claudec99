# ClaudeC99 Stage 53 Predefined macros __FILE__ and __LINE__

## Task
  Support standard predefined macros __FILE__ and __LINE__
  
## In-Scope
  - `__FILE__`
  - `__LINE__`
  - Expansion in ordinary source
  - Correct values in main source files
  - reasonable values inside include files
  - values should reflect preprocessed source location as closely as possible

## Rules
  - `__FILE__` expands to a string literal for the current source file.
  - `__LINE__` expands to an integer literal for the current source line.

## Out-of-scope
  - `__DATE__`
  - `__TIME__`
  - `__STDC__`
  - `__STDC_VERSION__`
  - `#line`
  - prefect dianostic/source-map support

## Tests
```C
---- helper.h ----
#ifndef __HELPER_H__
#define __HELPER_H__
#define NULL 0
int fileTest();
#endif
------------------
---- helper.c ____
#include "helper.h"
int puts(char *);
char * strchr(const char *, const char *);


int fileTest() {
    puts(__FILE__);
    return strcmp(strchr(__FILE__, "helper.c"), NULL) ;
}
-----------------
---- main .c test file ____
#include "helper.h"

int main() {
    return fileTest();   // expect status code: 0
}
```