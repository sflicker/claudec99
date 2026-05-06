# ClaudeC99 Stage 21-01 Function Declarator refactor

## Task
  - Refactor function declarations and function definitions so they use the
    general declarator machinery introduced in stage 20 instead of the old
    `<function_declarator>` grammar.
  - This stage should remove `<function_declarator>` as a distinct grammar production.


## Grammar Updates
Current
```ebnf
    <function> ::= <type_specifier> <function_declarator> ( <block_statement> | ";" )

    <function_declarator> ::= { "*" } <identifier> "(" [ <parameter_list> ] ")"
```

Updated
```ebnf

    <function> ::= <type_specifier> <declarator> ( <block_statement> | ";" )

```
The <declarator> used in <function> must declare a function. 
Examples
```C
    int f();
    int f() { return 1; }
    
    int add(int a, int b);
    int add(int a, int b) { return a + b; }
    
    int *identity(int *p);
    int *identity(int *p) { return p; }
```

## Requirements
  - Parse function prototypes using the general declarator path.
  - Parse function definitions using the general declarator path.
  - Remove special pointer handling from the old function-specific rule.
  - Preserve existing behavior for:
    - zero-argument functions
    - typed parameter lists
    - functions returning integer types
    - functions returning pointers
    - function calls
    - prototypes before definitions
  - Reject a function definition if the declarator does not have a function

## Important Semantic rule
This is value
```C
    int f() {
        return 1;
    }
```

This is invalid
```C
    int f {
        return 1;
    }
```
because `x` is an object declarator, not a function declarator

Out-of-scope
  - function pointers  `int (*fp)(int);`
  - Parenthesized declarators  `int (*f())();`
  - Array parameters `int f(int a[10]);` or `int f(int a[]);`
  - Mixed object/function declarations `int a, f();`
  - Multiple function declarators in one declaration `int g(), h();`
  - Full C meaning of empty parameter list

## Tests
  - Existing tests should continue to pass