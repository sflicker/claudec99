# Claude C99 Stage 10.1

## Task:
- Extend the compiler to support `break` and `continue`

## Definitions
- `break` exits the innermost enclosing loop immediately.
- `continue` skips the rest of the current loop body and begins the next iteration of the innermost enclosing loop. 

## Requirements
- add new tokens for the keywords
  - `break`
  - `continue`
  
- add `break` and `continue` support for EXISTING loop types:
  - for
    - `break` exits the body of the `for` loop
    - `continue` transfers control to the increment expression, then to the loop test.
  - while
    - `break` exits the body of the `while` loop
    - `continue` control passes to the `while` test

## Semantics
- both `break` and `continue` are only valid within an enclosing `for` or `while` loop
- Using either outside a loop is a compile time error.
  
## updated grammar
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
                    | <jump_statement>
                    | <expression_statement>

<declaration> ::= "int" <identifier> [ "=" <expression> ] ";"

<return_statement> ::= "return" <expression> ";"

<if_statement> ::= "if" "(" <expression> ")" <statement> [ "else" <statement> ]
 
<while_statement> ::= "while" "(" <expression> ")" <statement>

<for_statement> ::= "for" "(" [<expression>] ";" [<expression>] ";" [<expression>] ")" <statement>

<jump_statement> ::= "continue" ";"
                    | "break" ";"

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

## Testing
### Required tests
- `break` exits a `while`
- `break` exists a `for`
- `continue` in `while` skips remaining body and reevaluates condition
- `continue` in `for` skips remaining body and executes increment
- nested loops: `break` affects the innermost loop only
- nested loops: `continue` affects the innermost loop only
- compile-time error for `break` outside loop
- compile-time error for `continue` outside loop
