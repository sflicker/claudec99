# Claude C99 Stage 7.1

Task: Extend the compiler to support assignment expressions and compound assignment expressions.

## Requirements:

### Support the following new tokens:
- `+=`
- `-=`

### Updated Grammar

This stage will adds support for assignment expressions and compound assignment expressions.

```ebnf
<program> ::= <function>

<function> ::= "int" <identifier> "(" ")" <block_statement>

<block_statement> ::= "{" { <statement> } "}"

<statement> ::=  <declaration>
                    | <return_statement>
                    | <if_statement>
                    | <while_statement>
                    | <block_statement>
                    | <expression_statement>

<declaration> ::= "int" <identifier> [ "=" <expression> ] ";"

<return_statement> ::= "return" <expression> ";"

<if_statement> ::= "if" "(" <expression> ")" <statement> [ "else" <statement> ]
 
<while_statement> ::= "while" "(" <expression> ")" <statement>

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
- Assignment is now an expression.
- The operators `=`, `+=`, and `-=` may appear anywhere an expression is allowed.
- An assignment expression:
  - evaluates the right-hand expression
  - stores the result in the left-hand variable
  - evaluates to the final value stored in the left-hand variable
- Compound assignment:
  - `a += b` is equivalent to `a = a + b`
  - `a -= b` is equivalent to `a = a - b`
- Expressions statement are supported. Examples:
  - a = 5;
  - a = a + 1;
  - a += 2;
  - a -= 3;
- The left-hand side of an assignment must be a simple identifier in this stage.
- Chained assignment such as `a = b = 1` are not supported in this stage.

## Additional tests
- A number of additional c program files have already been added to the test/valid folder.