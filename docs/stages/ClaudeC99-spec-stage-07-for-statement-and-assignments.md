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

This stage add support for for loops

```ebnf
<program> ::= <function>

<function> ::= "int" <identifier> "(" ")" <block_statement>

<block_statement> ::= "{" { <statement> } "}"

<statement> ::=  <declaration>
                    | <assignment_statement>
                    | <return_statement>
                    | <if_statement>
                    | <while_statement>
                    | <for_statement>
                    | <block_statement>
                    | <expression_statement>

<declaration> ::= "int" <identifier> [ "=" <expression> ] ";"

<assignment_statement> ::= <assignment_expression> ";"

<assignment_expression> ::= <identifier> "=" <expression>
                           | <identifier> "+=" <expression>
                           | <identifier> "-=" <expression>
 
<return_statement>     ::= "return" <expression> ";"

<if_statement> ::= "if" "(" <expression> ")" <statement> [ "else" <statement> ]
 
<while_statement> ::= "while" "(" <expression> ")" <statement>

<for_statement> ::= "for" "(" [<expression>] ";" [<expression>] ";" [<expression>] ")" <statement>

<expression_statement> ::= <expression> ";"

<expression> ::= <equality_expression>

<equality_expression> ::= <relational_expression> [ ("==" | "!=") <relational_expression> ]*

<relational_expression> ::= <additive_expression> [ ( "<" | "<=" | ">" | ">=") <additive_expression> ]*

<additive_expression> ::= <multiplicative-expression> [ ("+" | "-") <multiplicative-expression> ]*

<multiplicative_expression> ::= <unary_expression> [ ("*" | "/") <unary_expression> ]*
   
<unary_expression> ::= [ "+" | "-" | "!" | "++" | "--" ] <unary_expression> | <primary_expression>

<postfix_expression> ::= <primary_expression> [ "++" | "--" ]?

<primary_expression>     ::= <int_literal> 
                         | <identifier>
                         | "(" <expression> ")"

<identifier>    ::= [a-zA-Z_][a-zA-Z0-9_]*

<int_literal> ::= [0-9]+

```