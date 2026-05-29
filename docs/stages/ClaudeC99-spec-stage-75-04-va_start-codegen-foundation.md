# ClaudeC99 - stage 75-04 va_start codegen foundation

## Goal:
Implement code generation fro __builtin_va_start and __buildin_va_end
for integer/pointer-only variadic functions on the current x86-64 SysV API target

## task
  - add several declarations for variable argument functions to stdio.h
  - for variadic function definitions allocate a hidden 304 byte register save area 
    - save registers rdi, rsi, rdx, rcx, r8, r9 into the register save area
  - save incoming GP argument registers in variadic function protocol
  - implement __builtin_va_start(ap, last)
  - initialize va_list_fields
    - ga_offset
    - fp_offset
    - overflow_arg_area
    - reg_save_area
  - implement __buildin_va_end(ap) as no-op

## Out of scope
  - va_arg
  - va_copy
  - floating point variadic arguments
  - full stdarg API edge cases

## updates to <project-root>/test/include/stdio.h>
Add the following declarations and include <stdarg.h>
```C
    ...
    #include <stdarg.h>

    ...

    int vfprint(FILE *, const char *, va_list);
    int vprintf(const char *, va_list);
    int vsprintf(char *, size_t, const char *, va_list);
```

## For X86-64 SysV, va_list is effectively an array of one struct with these fields
```C
typedef struct {
    unsigned int gp_offset;    // byte offset into reg_save_area for next saved general-purpose argument register
    unsigend int fp_offset;
    void *overflow_arg_area;   // address of the first stack-passed argument
    void *reg_save_area;
} va_list[1];
```

**1. variadic functions need a hidden register save area**
for any function definition marked is_variadic, reserve stack space for a register save area.
Use the following sizes for the register save area
```text
reg_save_area  size = 304 bytes
    GP slots:  6 * 8    = 48 bytes
    FP slots: 16*16     = 256 bytes
    total:              = 304 bytes

```
Layout of register save area:
```text
reg_save_area + 0   saved rdi
reg_save_area + 8   saved rsi
reg_save_area + 16  saved rdx
reg_save_area + 24  saved rcx
reg_save_area + 32  saved r8
reg_save_area + 40  saved r9
reg_save_area + 48  beginning of FP/XMM area, currently unused but reserve to 304
```

## prologue for variadic functions should emit
```asm
; after rbp/rsp frame setup
mov [rbp - regsave_offset + 0], rdi
mov [rbp - regsave_offset + 8], rsi
mov [rbp - regsave_offset + 16], rdx
mov [rbp - regsave_offset + 24], rcx
mov [rbp - regsave_offset + 32], r8
mov [rbp - regsave_offset + 40], r9
```
Leave FP area uninitialized for this stage, since FP is out of scope

**2. va_start(ap, last) initializes the va_list object**
example
```C
void log(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
}
```
ap is an array-of-one struct. So codegen should write fields into ap[0]
```text
ap + 0   gp_offset           unsigned int
ap + 4   fp_offset           unsigned int
gp + 8   overflow_arg_area   void *
gp + 16  reg_save_area       void *
```

va_start(ap, last) should write:
```text
ap[0].gp_offset       = named_gp_arg_count * 8
ap[0].fp_offset       = 48
ap[0].overflow_arg_area = address of first stack-passed argument
ap[0].reg_save_area     = address of register save area
```

example:
```C
void log1(const char * fmt, ...);
```
has one named GP argument so `gp_offset = 1 * 8 = 8`

example
```C
void log3(int a, int b, const char *fmt, ...);
```
has three named GP arguments so `gp_offset = 3 * 8 = 24`

clamp to 48 if there are six or more named GP args `gp_offset = 6 * 8 = 48`

**3. overflow_arg_area**
This should point to the first stack-passed unnamed variadic argument

Practical first rule
```text
 overflow_arg_area = rbp + 16 + 8 * stack_named_arg_count
```
where
```text
rbp + 8     return address
rbp + 16    first stack-passed argument, i.e. overall argument 7
rbp + 24    overall argument 8
```
stack_named_arg_count is the number of named parameters beyond the first six GP slots
Examples
```C
void f(int a, ...);
```
named GP args: 1
named stack args: 0
first possible stack variadic arg starts at
rbp + 16
but actual first variadic arg may be in the register save area, because gp_offset = 8

```C
void f(int a, int b, int c, int d, int e, int f, int g, ...)
```
Named GP args: 6
named Stack args: 1
first stack variadic arg starts at
` fbp + 16 + 8 = rbp + 24`
and gp_offset = 48, meaning no saved GP registers args remain for unnamed args.

4. Caller side remains as earlier stages

## Test
```C
------------ main test .c --------------
#include <stdarg.h>
#include <stdio.h>

void printv(const char *fmt, va_list ap) {
    vprintf(fmt, ap);
}

void print(const * fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    printv(fmt, ap);
    va_end(ap);
}

int main() {
    print("%d %d\n", 40, 2);
    return 0;
}
---------------------------------------
-------------- .expected --------------
40 2
---------------------------------------
```

