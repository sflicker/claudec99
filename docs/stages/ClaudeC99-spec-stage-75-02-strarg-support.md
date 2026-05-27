# ClaudeC99 stage 75-02 add stdard.h and va_list surface; macros may be present by unused 

## Task
  - add stdarg.h to <project-root>/test/include

controlled stdarg.h -> <project-root>/test/include
```C
----------- stdarg.h -------------------
#ifndef CLAUDEC99_STDARG_H
#define CLAUDEC99_STDARG_H

typedef struct __claudec00_va_list_tag {
    unsigned int gp_offset;
    unsigned int fp_offset;
    void *overflow_arg_area;
    void *reg_save_area;
} va_list[1];

#define va_start(ap, last)     __buildin_va_start(ap, last)
#define va_end(ap)             __builtin_va_end(ap)
#define va_copy(dst, src)      __builtin_va_copy(dst, src)
#define va_arg(ap, type)       __builtin_va_arg(ap, type)

#endif
---------------------------------------
```

Note the va_* macros in stdard.h are being defined to expand to the __buildin* s which don't
exist yet. This stage is only defining them in the macros however using those macros will be
out of scope.

## Out of Scope
  - using any of the va macros in stdarg.h
    - (va_start, va_end, va_copy, va_arg)
  - callee side handling of variadic arguments
  - register save area

## Tests
```C
#include <stdarg.h>

int main(void) {
    va_list ap;
    return sizeof(ap) > 0;     expect 1;
}
```

```C
#include <stdarg.h>

int helper(va_list ap) {
    return ap != 0;
}

int main(void) {
    va_list ap;
    return helper(ap);     expect 1;
}
```