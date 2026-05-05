# ClaudeC99 Stage-20-01  Declarator refactor

## Task
  - Refactor declarations internally so pointer, array and identifier binding lives 
     in the declarator instead of being baked into the grammar-level `<type>` or directly into the 
     declaration node.
  - This stage should preserve existing supported behavior while preparing for additional changes
    list comma-separated init-declarator lists
  - The key conceptual change is:
```C
    Before:
        int *p;
        
        <type> = int *
        identifier = p
        
    After:
        int *p;
        
        <type_specifier> = int
        <declarator> = *p
        final semantic type = int *
```
    - The compiler should still build semantic `type` objects internally. This refactor only changes how source
      syntax is parsed into those types.

## Grammar Updates
Existing to update
```ebnf

<declaration> ::= <type> <identifier> [ "[" [ <integer_literal> ] "]" ] [ "=" <expression> ] ";"

<type> ::= <integer_type> { "*" }

<function> ::= <type> <identifier> "(" [ <parameter_list> ] ")" ( <block_statement> | ";" )

<parameter_list> ::= <parameter_declaration> { "," <parameter_declaration> }

<parameter_declaration> ::= <type> <identifier>

<integer_type> ::= "char" | "short" | "int" | "long"

<cast_expression> ::= <unary_expression>
                    | "(" <type> ")" <cast_expression>

<unary_expression> ::= "sizeof" <unary_expression>
                    | "sizeof" "(" <type> ")"
                    | <unary_operator> <unary_expression>
                    | <postfix_expression>
```

Updated
```ebnf

<integer_type> ::= "char" | "short" | "int" | "long"

<type_specifier> ::= <integer_type> 

<type_name> ::= <type_specifier> {"*"}

<declaration> ::= <type_specifier> <init_declarator> ";"

<init_declarator> ::= <declarator> [ "=" <initializer_expression> ]

<initializer_expression> ::= <assignment_expression>                       

<declarator> ::= { "*" } <direct_declarator>

<direct_declarator> ::= <identifier> 
                       | <identifier> "[" [<integer_literal>] "]"

<function> ::= <type_specifier> <function_declarator> ( <block_statement> | ";" )

<function_declarator> ::= { "*" } <identifier> "(" [<parameter_list>] ")"

<parameter_list> ::= <parameter_declaration> { "," <parameter_declaration> }

<parameter_declaration> ::= <type_specifier> <parameter_declarator>

<parameter_declarator> ::= { "*" } <identifier>

<cast_expression> ::= "(" <type_name> ")" <cast_expression>
                      | <unary_expression>

<unary_expression> ::= <sizeof_expression>
                    | <unary_operator> <unary_expression>
                    | <postfix_expression>
                    
<sizeof_expression> ::= "sizeof" <unary_expression>
                        | "sizeof" "(" <type_name> ")"                    
```

## Notes
THe old grammar rule:
```ebnf
    <type> ::= <integer_type> { "*" }
```

is replaced by two separate concepts:
```ebnf
    <type_specifier> ::= <integer_type>
    <type_name> ::= <type_specifier> { "*" }
```

<type_specifier> is used where declarations start with a base type such as `int`, `char`, `short` or `long`

<type_name> is used for nameless type contexts such as casts and `sizeof(type)`
Examples:
```C
    (int)x
    (int *)x
    sizeof(int)
    sizeof(int *)
```

Named declarations now use declarators:
```C
    int x;
    int *p;
    int **p;
    char s[4];
```
In these cases, the `*`, identifier and array suffix belong to the declarator
For this stage only one declarator per declaration is supported. comma-separated declarations
are out of scope for this stage.

## AST Impact

This stage does not require a new AST node kind.

The parser may use an internal `ParsedDeclarator` helper structure while parsing declarations, parameters, and function declarators. The final AST should continue to store the computed semantic type for each variable, parameter, and function return value.

If array metadata is currently stored directly on declaration AST nodes, it may remain there for this stage unless moving it into the semantic `Type` representation is straightforward. The main requirement is that existing AST behavior and print-AST output remain compatible except where type formatting naturally reflects the same existing types.

## Codegen Impact

This stage should not intentionally change generated code behavior.

Codegen should continue to operate on the final semantic types attached to AST nodes. Most codegen changes, if any, should be limited to adapting to minor AST/type representation cleanup.

All existing pointer, array, cast, function return, parameter, `sizeof`, and initializer tests should continue to pass.


## Tests
All existing "Valid" tests should still pass