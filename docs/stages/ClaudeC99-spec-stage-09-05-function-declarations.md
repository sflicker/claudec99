# Claude C99 Stage 9.5

## Task:
- Extend the compiler to support function declarations

## Definitions
- A function declaration introduces a functions identifier and parameter list, does NOT include a function body. A function declaration ends with a semicolon.
- A function body also includes a function body.

## Requirements
- Function can be declared separately from their definitions
- Function declarations must be at file scope.
- Function calls are allowed if the declaration or definitions is visible at the call site.
- Function overriding is NOT allowed.
- Multiple external declarations in the same translation unit must be supported.
- Function calls remain supported as in the previous stage.
- If a function declaration and definition are present with the same identifier both must have the same parameter list.
- Both Function declarations and definitions appear at the root level of a translation unit.
- For this stage, only `int` return types and `int` parameter types are supported.

# Updated Grammar:

```ebnf
<translation-unit> ::= <external-declaration> { <external-declaration> }

<external-declaration> ::= <function>

<function> ::= "int" <identifier> "(" [ <parameter_list> ] ")" ( <block_statement> | ";" )

<parameter_list> ::= <parameter_declaration> { "," <parameter_declaration> }

<parameter_declaration> ::= "int" <identifier>

<block_statement> ::= "{" { <statement> } "}"

<statement> ::=  <declaration>
                    | <return_statement>
                    | <if_statement>
                    | <while_statement>
                    | <for_statement>
                    | <block_statement>
                    | <expression_statement>

<declaration> ::= "int" <identifier> [ "=" <expression> ] ";"

<return_statement> ::= "return" <expression> ";"

<if_statement> ::= "if" "(" <expression> ")" <statement> [ "else" <statement> ]
 
<while_statement> ::= "while" "(" <expression> ")" <statement>

<for_statement> ::= "for" "(" [<expression>] ";" [<expression>] ";" [<expression>] ")" <statement>

<expression_statement> ::= <expression> ";"

<expression> ::= <assignment_expression>

<assignment_expression> ::= <identifier> "=" <assignment_expression>
                           | <identifier> "+=" <assignment_expression>
                           | <identifier> "-=" <assignment_expression>
                           | <logical_or_expression>
 
<logical_or_expression> ::= <logical_and_expression> { "||" <logical_and_expression> }
 
<logical_and_expression> ::= <equality_expression> { "&&" <equality_expression> }
 
<equality_expression> ::= <relational_expression> { ("==" | "!=") <relational_expression> }

<relational_expression> ::= <additive_expression> { ( "<" | "<=" | ">" | ">=") <additive_expression> }

<additive_expression> ::= <multiplicative_expression> { ("+" | "-") <multiplicative_expression> }

<multiplicative_expression> ::= <unary_expression> { ("*" | "/") <unary_expression> }
   
<unary_expression> ::= ( "+" | "-" | "!" | "++" | "--" ) <unary_expression>  
                    | <postfix_expression>
                    
<postfix_expression> ::= <primary_expression> { "++" | "--" }                    

<primary_expression> ::= <int_literal> 
                         | <identifier>
                         | <function_call>
                         | "(" <expression> ")"

<function_call> ::= <identifier> "(" [ <argument-expression-list> ] ")"

<argument-expression-list> ::= <assignment_expression> { "," <assignment_expression> }

<identifier> ::= [a-zA-Z_][a-zA-Z0-9_]*

<int_literal> ::= [0-9]+
```

## Notes for this stage
- A function declaration does not generate code.
- A function definition does generate code.
- Function declarations should be represented in the AST or symbol table so 
   later stages can validate function calls against there signatures.
- For this stage, it is acceptable to require that all declarations and definitions
    for the same function use matching parameter counts.

