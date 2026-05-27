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

