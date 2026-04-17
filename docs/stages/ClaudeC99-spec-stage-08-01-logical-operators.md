# Claude C99 Stage 8.1

Task: Extend the compiler to support logical operators `&&` and `||`.

## Requirements:

### Support the following new tokens:
- `&&`
- `||`

### Semantics
- `&&` and `||` must evaluate to an integer result:
  - false = `0`
  - true = `1`
- These operators must use **short-circuit evaluation**:
  - for `a && b`, if `a` evaluates to false, `b` is not evaluated.
  - for `a || b`, if `a` evaluates to true, `b` is not evaluated.
- Operator precedence must match C-style precedence:
  - `||` lower than `&&`
  - both lower than equality and relation operators

### Updated Grammar
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

<int_literal> ::= [0-9]+c

```

## Notes
- This stage adds parsing and code generation for logical `&&` and `||`.
- Existing comparison operators already produce integer truth values suitable for use with logical operators.
- The compiler must generate labels/branches as needed to implement short-circuit behavior.
