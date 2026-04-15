# Claude C99 Stage 7

Task: Extend the compiler to support for loops and additional assignment forms.

## Requirements:

### Support the following new tokens:
- `for`
- `+=`
- `-=`
- `++`
- `--`

### Updated Grammar

This stage adds support for `for` loops

```ebnf

<program> ::= <function>

<function> ::= "int" <identifier> "(" ")" <block_statement>

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
                           | <equality_expression>
 
<equality_expression> ::= <relational_expression> [ ("==" | "!=") <relational_expression> ]*

<relational_expression> ::= <additive_expression> [ ( "<" | "<=" | ">" | ">=") <additive_expression> ]*

<additive_expression> ::= <multiplicative_expression> [ ("+" | "-") <multiplicative_expression> ]*

<multiplicative_expression> ::= <unary_expression> [ ("*" | "/") <unary_expression> ]*
   
<unary_expression> ::= [ "+" | "-" | "!" ] <unary_expression> 
                    | <primary_expression>

<primary_expression>     ::= <int_literal> 
                         | <identifier>
                         | "(" <expression> ")"

<identifier>    ::= [a-zA-Z_][a-zA-Z0-9_]*

<int_literal> ::= [0-9]+
```
## Semantics
`for` statements are of the form
     for (initializer ; condition ; update) block
- the initializer is called one time at the beginning of execution of the for loop
- For this stage variables must be declared before executing the initialize
- the condition is called before every iteration of the loop
  - if the condition is evaluates to truthy (non-zero) the loop is executed
  - if the condition is zero the statement exits
- after each iteration of block the update is called
