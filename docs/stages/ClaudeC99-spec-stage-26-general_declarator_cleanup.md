# ClaudeC99 Stage 26 General Declarator Cleanup

## Task
  Refactor declarator parsing so function pointer declarations are handled by
  the same recursive declarator machinery as orginary identifiers, pointers,
  arrays and function declarators.
  
## Goals
  - Replace identifier-specific direct declarator suffixes with recursive direct declarator suffixes
  - Remove the special <func_ptr_declarator> grammar path
  - Continue supporting existing function pointer declarators, initialization,
    assignment, and indirect calls from stage 25.
  - Allow unnamed parameter declarations in function prototypes and function pointer signatures
  - keep function definition parameters named.

## Grammar Updates

Update:
```ebnf
    <declarator> ::= { "*" } <direct_declarator>
    
    <direct_declarator> ::= <identifier>
                         | "(" <declarator> ")"
                         | <direct_declarator> "[" [ <integer_literal> ] "]"
                         | <direct_declarator> "(" [ <parameter_list> ] ")"
                         
    <parameter_declarator> ::= <type_specifier> [ <declarator> ]
    
Removing:
    <func_ptr_declarator>
    <func_ptr_param_list>
    <func_ptr_param>                         
```

## Out of Scope
  - typedef
  - enum
  - struct
  - arrays of function pointers
  - pointer-to-pointer-to-function
  - functions return function pointers
  - full abstract declarators
  - declaration specifier generalization

## Tests
  - existing tests should continue to pass. If necessary some may be adjusted if necessary.
