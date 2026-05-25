# ClaudeC99 Stage 68 - support more than 6 arguments in a method call

## Task
  - Add support for method calls with more than 6 arguments.
  - Function call codegen should follow the x86-64 System V ABI for 
    functions with more than six integer/pointer arguments by passing
    arguments 7+ on the stack.\
  - Currently the compiler supports calls with up to 6 arguments. Stage
    68 should remove that limit for integer, pointer, _Bool, enum and struct-pointer arguments.
  
## Requirements
This stage will support passing arguments of the following types as 7th+ on the stack
```C
char/short/int/long/long long
signed/unsigned variants
_Bool
enum constants
pointers
void *
struct pointers
function pointers
string literals
```

**Parser/semantic limit removal**
Remove hard-coded argument count checks for maximum 6 arguments
Other validation rules should remain

**Stack-passed argument for calls**
For SysV/AMD64 integer/pointer args
```text
arg1 -> rdi
arg2 -> rsi
arg3 -> rdx
arg4 -> rcx
arg5 -> r8
arg6 -> r9
arg7+ -> stack
```

**Stack arguments are placed so the callee sees**
```asm
[rbp + 16] first stack argument / arg7
[rbp + 24] arg8
[rbp + 32] arg9
...
```
inside a normal frame-pointer function.

**Caller-side strategy**
```Text
evaluate arguments
preserve them somewhere
move first six arguments into registers
push stack remaining arguments **right-to-left** 
call function
clean stack arguments after call
```

**Example Call**
```C
f(1,2,3,4,5,6,7,8);
```
Caller should effectively arrange
```text
rdi = 1
rsi = 2
rdx = 3
rcx = 4
r8 = 5
r9 = 6
stack arg7 = 7   ; [rbp + 16]
stack arg8 = 8   ; [rbp + 24]
```
Push order should be
```Text
push arg8
push arg7
call f
add rsp, 16   
```

**Stack Alignment before Call**
SysV AMD64 requires stack alignment around call. Before the call, the stack should
be aligned so the called gets the expected API layout.
Practical Rule For Compiler
```Text
Before Call: rsp should be 16-byte aligned
After call pushes return address: callee sees rsp % 16 == 8
```
If you push an odd number of 8-byte stack arguments, alignment may need padding

**Example**
```Text
7 args total -> 1 stack push
push padding
push arg7
call
add rsp, 16
```
```Text
8 args total -> 2 stack args
push arg8
push arg7
call
add rsp, 16
```
This is especially important for libc calls

**Callee side parameter access*
Parameters 1-6 will still come from registers. Parameters 7+ are read from stack slots in the caller's frame.

Inside callee frame pointer
```text
arg7 at [rbp + 16]
arg8 at [rbp + 24]
arg9 at [rbp + 32]
...
```
codegen implementation cal either
1. load stack parameters directly from positive rbp offsets, or
2. copy them into local stack slots in the prologue like register parameters

**Variadic external calls with 7+ arguments should work**
Example
```C
#include <stdio.h>
int main(void) {
    printf("%d %d %d %d %d %d %d\n", 1, 2, 3, 4, 5, 6, 7);
    return 0;    // EXPECTED OUTPUT SHOULD BE: 1 2 3 4 5 6 7
}
```

## Out of scope
  - floating point arguments
  - struct-by-value arguments
  - variadic function definitions
  - full va_arg / stdarg.h
  - Windows X64 API

## Tests
seven argument test
```C
int f(int a, int b, int c, int d, int e, int f, int g) {
    return g;
}

int main(void) {
    return f(0,0,0,0,0,0,42);   // expect 42
}
```

eight argument test
```C
int f(int a, int b, int c, int d, int e, int f, int g, int h) {
    return a + b + c + d + e + f + g + h;
}

int main() {
    return f(1,2,3,4,5,6,7,14);  // expect 42
}
```

stack arg with expression
```C
int f(int a, int b, int c, int d, int e, int f, int g) {
    return g;
}

int main(void) {
    int x;
    x = 40;
    return f(0,0,0,0,0,0,x + 2);   // expect 42
}
```

pointer stack argument
```C
int f(int a, int b, int c, int d, int e, int f, int *p) {
    return *p;
}

int main(void) {
    int x;
    x = 42;
    return f(0,0,0,0,0,0,&x);   // expect 42
}
```

string literal stack argument
```C
#include <string.h>

int check(int a, int b, int c, int d, int e, int f, const char *s) {
    return strcmp(s, "hello") == 0;
}

int main(void) {
    return check(1,2,3,4,5,6,"hello");   // expet 1
}
```

external Variadic call with stack args (integration test)
```C
--------- main test .c -------------
#include <stdio.h>

int main(void) {
    printf("%d %d %d %d %d %d %d\n", 1,2,3,4,5,6,7);
    return 0;   // expect 0
}
------------------------------------

-------- .expected -----------------
1 2 3 4 5 6 7
------------------------------------
```

Many args with mixed integer widths
```C
long long sum7(int a, short b, char c, long d, long long e, unsigned int 7, unsigned long long g) {
    return a + b + c + d + e + f + g;
}

int main(void) {
    return sum7(1, 2, 3, 4L, 5LL, 6U, 21ULL);     // expect 42
}
```
