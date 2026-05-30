# ClaudeC99 75-06 - va_arg for integer and pointer types

## goal
  - implement code generation for
    - `va_arg(ap, type)` after macro expansion to __builtin_va_arg(ap, type)
  - for integer and pointer types on the current x86-64 ABI target

## In-Scope
support va_arg for general purpose register class values:
```C
va_arg(ap, int)
va_arg(ap, unsigned int)
va_arg(ap, long)
va_arg(ap, unsigned long)
va_arg(ap, long long)
va_arg(ap, unsigned long long)
va_arg(ap, char *)
va_arg(ap, void *)
va_arg(ap, struct Token *)
va_arg(ap, int(*)(int))
```

## ABI model
using the existing va_list layout:
```C
typedef struct __claudeC99_va_list_arg {
    unsigned int gp_offset;
    unsigned int fp_offset;
    void *overflow_arg_area;
    void *reg_save_area;
} va_list[1];
```

Field Offsets:
```C
ap[0].gp_offset           offset 0, size 4
ap[0].fp_offset           offset 4, size 4
ap[0].overflow_arg_area   offset 8, size 8
ap[0].reg_save_arga       offset 16, size 8
```

for integer/pointer only va_arg:
```C
if ap[0].gp_offset < 48:
    source address = ap[0].reg_save_area + ap[0].gp_offset
    ap[0].gp_offset += 8
else:
    source address = ap[0].overflow_arg_area
    ap[0].overflow_arg_area += 8
```
48 is size 8 byte GP argument slots:
```text
    rdi, rsi, rdx, rcx, r8, r9
```

**Key semantic point**
va_arg does not know when the argument list ends.

Functions using va_arg must have a separate protocol to know when the argument list ends.

## Parser
   __builtin_va_arg(ap, int) was already implemented in a previous stage so no updates needed.
   
## Codegen
   - This stage adds codegen support for __builtin_va_arg. The AST node will have two arguments
   - a. an expression b. a type
   - Only integer/GP related types will be supported in this stage an include
   ```c
       int / unsigned int
       long /unsinged long
       long long / unsigned long long
       pointers
       void *
       function pointers
       struct/union pointer
   ```
   - types not support at this stage 
    ```c
        float
        double
        struct by value
        union by value
        array types
        void
    ```
    also small types are not supported at this stage
    ```
       int
       short
       _Bool
    ```
    Users should use casting instead of small types as va_arg type arguments

## Codegen algorithm
given
```C
    value = va_arg(ap, T);
```
generate code equivalent to:
```c
   ap_addr = address of a[0]
   
   gp_offset = *(unsigned int *)(ap_addr + 0)
   
   if gp_offset < 48:
       reg_save_area = *(void **)(ap_addr + 16)
       src = reg_save_area + gp_offset
       *(unsigned int *)(ap_addr + 0) = gp_offset + 8
   else:
       src = *(void**)(ap_addr + 8)
       *(void **)(ap_addr + 8) = src + 8
       
   load value of requested type T from src
```
Load Behavior:
```C
    int / unsigned int               load 4 bytes, size/zero extend appropriately
    long / unsigned long             load 8 bytes
    long long / unsigned long long   load 8 bytes
    pointer types                    load 8 bytes  
```
Even though each GP vararg slot is 8 bytes, an int argument should be read as an int value
from that slot and represented according to ClaudeC99's normal type rules.

**Suggested pseudo-assembly shape
(conceptual only)
```asm
; rbx = address of ap[0]

   mov eax, dword [ rbx + 0 ]        ; gp_offset
   cmp eax, 48
   jae .from_overflow

.from_reg_save:
    mov rcx, [rbx + 16]               ; reg_save_area
    lea rdx, [rcx + rax]              ; src
    add eax, 8                        ; advance offset
    mov [rbx + 0], eax                ; set gp_offset to new offset
    jmp .load
    
.from_overflow:
    mov rdx, [rbx + 8]                ; src = overflow_arg_area
    lea rcx, [rdx + 8]                ; advance offset
    mov [rbx + 8], rcx                ; set overflow_arg_area to advance offset
    
.load
    ; from load requested type from [rdx]

```
example type-specific load conceptual
```asm
    ; int
    movsxd rax, dword [rdx]
    
    ; unsigned int
    mov eax, dword [rdx]
    
    ; long /pointer
    
    mov rax, [rdx]
```


## Tests
Basic int args from register save area
```C
#include <stdarg.h>

int sum3(int first, ...) {
    va_list ap;
    int total;

    total = first;
    va_start(ap, first);

    total += va_arg(ap, int);
    total += va_arg(ap, int);
    va_end(ap);
    
    return total;
}

int main(void) {
    return sum3(10, 20, 12);   // expect 42
}
```

Transition from register save area to overflow stack area
```C
#include <stdarg.h>

int pick7(int fixed, ...) {
    va_list ap;
    int a;
    int b;
    int c;
    int d;
    int e;
    int f;
    int g;
    
    va_start(ap, fixed);
    
    a = va_arg(ap, int);
    b = va_arg(ap, int);
    c = va_arg(ap, int);
    d = va_arg(ap, int);
    e = va_arg(ap, int);
    f = va_arg(ap, int);
    g = va_arg(ap, int);
    
    va_end(ap);
    
    return g;
}

int main(void) {
    return(0, 1, 2, 3, 4, 5, 6, 42);    // expect 42
}
```

Pointer argument
```C
#include <stdarg.h>

int read_ptr(int ignored, ...) {
    va_list ap;
    int *p;
    
    va_start(ap, ignored);
    p = va_arg(ap, int *);
    va_end(ap);
    
    return *p;
}

int main(void) {
    int x;
    x = 42;
    return read_ptr(0, &x);    // expect 42
}
```

string pointer argument
```C
#include <stdarg.h>
#include <string.h>

int check(int ignored, ...) {
    va_list ap;
    const char *s;
    
    va_start(ap, ignored);
    s = va_arg(ap, const char *);
    va_end(ap);
    
    return strcmp(s, "hello") == 0;
}

int main() {
    check(0, "hello");    // expect 1
}
```

long long
```C
#include <stdarg.h>

int check(int ignored, ...) {
    va_list ap;
    long long x;
    
    va_start(ap, ignored);
    x = va_arg(ap, long long);
    va_end(ap);
    
    return x = 42LL;
}

int main(void) {
    return check(42LL);    // expect 1
}
```

multiple types
```C
#include <stdarg.h>

int mixed(int ignored, ...) {
    va_list ap;
    int a;
    long b;
    unsigned long long c;
    
    va_start(ap, ignored);
    
    a = va_arg(ap, int);
    b = va_arg(ap, long);
    c = va_arg(ap, unsigned long long);
    
    va_end(ap);
    
    return a + b + c;
}

int main(void) {
    return mixed(0, 10, 20L, 12ULL);  // expect 42
}
```

## Invalid tests
reject va_arg with unsupported small promoted type
```C
#include <stdarg.h>

int f(int ignored, ...) {
    va_list ap;
    char c;
    
    va_start(ap, ignored);
    c = va_arg(ap, char);    // envalid;
    va_end(ap);
    
    return c;
}

int main() {
    f(0);
}
```

reject aggregate by value
```C
include <stdarg.h>

struct Point {
    int x;
    int y;
};

int f(int ignored, ...) {
    va_list ap;
    struct Point p;
    
    va_start(ap, ignored);
    p = va_arg(ap, struct Point);   //INVALID
    va_end(ap);
    
    return p.x;
}

int main() {
    f(0);
}
```