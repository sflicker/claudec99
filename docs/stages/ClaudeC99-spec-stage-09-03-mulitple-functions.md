# Claude C99 Stage 9.3

## Task: 
- Extend the compiler to support function definitions with parameters lists along with multiple function definitions in the same file. 

## Requirements:
- Add support for function parameter lists.
- Add support for translation units containing one or more function definitions.
- Only function definitions are supported in this stage.
- Function forward declarations /prototypes are not support in this stage.
- Function calls are not supported in this stage.
- Only int type is supported for parameters.
- Only int type is supported for function return values.
- Add support for the comma token "," as needed for parameter lists.

## Semantic Rules
- Parameter names must be unique within a function's parameter list.
- Function names must be unique within the translation unit.
- Parameters are in scope throughout the function body.
- Parameters behave like local variables for expression use.
- Redeclaring a parameter name in the same function scope is not allowed.
- Since function calls are not yet supported, this stage only requires:
  - parsing parameter lists
  - storing parameter information in the AST / Symbol structures
  - making parameters available for use inside the function body
  - generating code so parameter values can be accessed within the function

# Token updates (add comma token)
- `,`

# Updated Grammar:

```ebnf 
<translation-unit> ::= <external-declaration> { <external-declaration> }
                     
<external-declaration> ::= <function>

<function> ::= "int" <identifier> "(" [ <parameter_list> ] ")" <block_statement>

<parameter_list> ::= <parameter_declaration> { ',' <parameter_declaration> }
                     
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
                         | "(" <expression> ")"

<identifier> ::= [a-zA-Z_][a-zA-Z0-9_]*

<int_literal> ::= [0-9]+
```