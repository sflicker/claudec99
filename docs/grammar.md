```ebnf
# Claude C99 Grammar (Current)

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
                    | <do_while_statement>
                    | <for_statement>
                    | <switch_statement>
                    | <labeled_statement>
                    | <block_statement>
                    | <jump_statement>
                    | <expression_statement>

<declaration> ::= "int" <identifier> [ "=" <expression> ] ";"

<return_statement> ::= "return" <expression> ";"

<if_statement> ::= "if" "(" <expression> ")" <statement> [ "else" <statement> ]
 
<while_statement> ::= "while" "(" <expression> ")" <statement>

<do_while_statement> ::= "do" <statement> "while" "(" <expression> ")" ";"

<for_statement> ::= "for" "(" [<expression>] ";" [<expression>] ";" [<expression>] ")" <statement>

<switch_statement> ::= "switch" "(" <expression> ")" <statement>

<labeled_statement> ::= "case" <constant_expression> ":" <statement>
                      | "default" ":" <statement>

<constant_expression> ::= <integer_literal>

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

<primary_expression> ::= <integer_literal>
                         | <identifier>
                         | <function_call>
                         | "(" <expression> ")"

<function_call> ::= <identifier> "(" [ <argument-expression-list> ] ")"

<argument-expression-list> ::= <assignment_expression> { "," <assignment_expression> }

<identifier> ::= [a-zA-Z_][a-zA-Z0-9_]*

<integer_literal> ::= [0-9]+

```
