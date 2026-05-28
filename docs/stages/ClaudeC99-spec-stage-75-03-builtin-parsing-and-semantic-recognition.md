# ClaudeC99 stage 75-02 stdard __builtin* parsing and semantic recognition

## Goal
  - recognize the builtin forms produced by controlled <stdarg.h> macros, without implementing real 
    variadic argument access or ABI setup yet.

## In Scope
Recognize and parse
```C
__builtin_va_start(ap, last)
__builtln_va_end(ap)
__builtin_va_copy(dst, src)
__builtin_va_arg(ap, type)
```

## AST Nodes types for builtins
Create a AST Node type for each builtin
```C
   AST_BUILTIN_VA_START
   AST_BUILTIN_VA_END
   AST_BUILTIN_VA_COPY
   AST_BUILTIN_VA_ARG
```

A flag like isBuiltin may be placed in the ASTNode struct

## Out of scope
Do not implement yet
  - real va_start ABI initialization
  - register save area
  - va_arg runtime extraction
  - va_copy runtime copying
  - passing va_list to vfprint/vsnprintf successfully
  - full SysV va_list behavior

## Parser
   Should recognize the forms and create appropriate ASTNode
   
## Specific builtin rules

__buildin_va_start(ap, fmt)
  - exactly 2 arguments
  - first argument must be an expression
  - second argument must ben an expression or name

Semantic Rule
   Must appear inside a variadic function

__builtin_va_end(ap)
   - exactly 1 argument
   - argument should be va_list-compatible expression

__builtin_va_copy(dst, src)
  - exactly two arguments
  - both should be va_list-compatible expressions

__builtin_va_arg(ap, int)
  - first operand is expression
  - second operand is type-name

Grammar Rule

```ebnf
<builtin_expression> ::=
     "__builtin_va_start" "(" <expression> "," <expression> ")"
   | "__buildin_va_end"   "(" <expression> ")"
   | "__buildin_va_copy"  "(" <expression> "," <expression> ")"
   | "__buildin_va_arg"   "(" <expression> "," <type_name> ")"
```

## Codegen
For this stage __builtin_* are not supported. Should handle as a no-op for this stage

## Tests (this stage will only add token, AST and invalid tests since code gen is not supported)

## Create token and AST tests
```C
#include <stdarg.h>

int f(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    va_end(ap);
    return 42;
}

int main(void) {
    return f("%d", 1);
}
```

```C
#include <stdarg.h>

int f(int first, ...) {
    va_list ap;
    int x;
    
    va_start(ap, first);
    x = va_arg(ap, int);
    va_end(ap);
    
    return x;
}
```

## Invalid tests
Reject va_start outside a variadic function
```c
#include <stdarg.h>

int f(const char *fmt) {
    va_list ap;
    va_start(ap, fmt);    // INVALID [outside a variadic function]
    va_end(ap);
    return 0;
}
```

bad argument count
```C
#include <stdarg.h>

int f(int first, ...) {
    va_list ap;
    va_start(ap);      // INVALID   [va_start should have two arguments]
    return 0;
```

va_arg requires type operand
```C
#include <stdarg.h>

int f(int first, ...) {
    va_list ap;
    int x;
    int y;
    
    va_start(ap, first);
    x = va_arg(ap, y);      // INVALID [second arg should be a type]
    va_end(ap);
    
    return x;
}
```