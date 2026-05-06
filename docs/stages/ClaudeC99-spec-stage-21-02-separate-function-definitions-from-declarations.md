# ClaudeC99 Stage 21-02 Separate functions definitions from declarations

## Task
  - Refactor external declarations so function definitions and ordinary declarations are separate grammar paths.
  - Function prototypes should be parsed as declarations with function declarators,
    not as function definitions without bodies.

## Grammar updates
Current:
```ebnf
    <external-declaration> ::= <function>

    <function> ::= <type_specifier> <declarator> ( <block_statement> | ";" )
```

Updated:
```ebnf
    <external_declaration> ::= <function>
                              | <declaration>
                              
    <function_definition> ::= <type_specifier> <declarator> <block_statement>
    
    <declaration> ::- <type_specifier> <init_declarator_list> ";"
    
    <init_declaration_list> ::= <init_declarator> { "," <init_declarator> }
    
    <init_declarator> ::= <declarator> [ "=" <expression> ]                             
```

## Semantic rule
  - The declarator in a <function_definition> must resolve to a function type.
  - The <declarator> in a file scope <declaration> may resolve to a function type
    for prototypes.

## Examples
Function definition:
```C
    int add(int a, int b) {
        return a + b;
    }
```
Function prototype:
```C
    int add(int a, int b);
```

## Requirements
  - Parse function definitions through `<function_definition>`
  - Parse prototypes through `<declarations>`
  - Preserve existing prototype-before-definition behavior
  - Preserve function type checking
    - return type compatibility
    - parameter count compatibility
    - parameter type compatibility
    - duplicate definition errors
  - Do not allow initializer on function declarations

## Valid Tests
```C
    int add(int a, int b);
    
    int main() {
        return add(2, 3);
    }
    
    int add(int a, int b) {
        return a + b;
    }
```

```C
    int *identity(int *p);
    
    int main() {
        int x = 7;
        int *p = identity(&x);
        return *p;
    }
    
    int *identity(int *p) {
        return p;
    }
```

## Invalid Tests
Function initializer:
```C
    int f() = 3;     // INVALID
```

Prototype/definition mismatch
```C
    int f(int a);
    int f(int a, int b) {        //INVALID
        return a + b;
    }
```

Duplicate definition:
```C
    int f() {
        return 1;
    }
    
    int f() {            // INVALID
        return 2;
    }
```

Non-function definition
```C
    int x {          // INVALID
        return ;
    }
```

## Out of scope
  - file scope variable storage
  - function pointers
  - mixed object, function declarations
  - multiple function prototypes in one declaration
  - old style c empty parameter list semantics
