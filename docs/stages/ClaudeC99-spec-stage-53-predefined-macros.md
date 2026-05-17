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
int fileTest();
------------------
---- helper.c ____
#include "helper.h"
int puts(char *);

int fileTest() {
    puts(__FILE__);
    puts(__LINE__);
    return strcmp(__FILE__, "helper.c");
}
-----------------
---- main .c test file ____
#include "helper.h"

int main() {
    return fileTest();   // expect status code: 0
}
```