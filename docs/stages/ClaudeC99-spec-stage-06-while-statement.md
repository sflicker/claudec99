# Claude C99 Stage 6

Task: extend the compiler to support while loops.

## Requirements:

### Support the following new tokens:
- `while`

### Updated Grammar

This stage adds support for while loops

```ebnf
<program> ::= <function>

<function> ::= "int" <identifier> "(" ")" <block_statement>

<block_statement> ::= "{" { <statement> } "}"

<statement> ::=  <declaration>
                    | <assignment_statement>
                    | <return_statement>
                    | <if_statement>
                    | <while_statement>
                    | <block_statement>
                    | <expression_statement>

<declaration> ::= "int" <identifier> [ "=" <expression> ] ";"

<assignment_statement> ::= <identifier> "=" <expression> ";"
 
<return_statement>     ::= "return" <expression> ";"

<if_statement> ::= "if" "(" <expression> ")" <statement> [ "else" <statement> ]
 
<while_statement> ::= "while" "(" <expression> ")" <statement>

<expression_statement> ::= <expression> ";"

<expression> ::= <equality_expression>

<equality_expression> ::= <relational_expression> [ ("==" | "!=") <relational_expression> ]*

<relational_expression> ::= <additive_expression> [ ( "<" | "<=" | ">" | ">=") <additive_expression> ]*

<additive_expression> ::= <multiplicative-expression> [ ("+" | "-") <multiplicative-expression> ]*

<multiplicative_expression> ::= <unary_expression> [ ("*" | "/") <unary_expression> ]*
   
<unary_expression> ::= [ "+" | "-" | "!" ] <unary_expression> | <primary_expression>

<primary_expression>     ::= <int_literal> 
                         | <identifier>
                         | "(" <expression> ")"

<identifier>    ::= [a-zA-Z_][a-zA-Z0-9_]*

<int_literal> ::= [0-9]+

```

## Semantics
- A `while` loop repeatedly evaluates its condition expression before each iteration.
- if the expression evaluates to 0, the look terminates and execution continues after the loop body.
- if the expression evaluates to nonzero, the loop body executes, then the condition is evaluated again.
- The loop body is a single <statement>, which may be either a simple statement or a block statement.

## AST and Parser Updates
- add a new AST type to support while statements.
- The node should store:
  - the loop condition expression
  - the loop body statement
- `while` statements should parse as a statement form, similar to `if`.

## Codegen notes
- Generate a loop start label.
- evaluate the condition at the top of each iteration.
- If the condition is false (`0`), jump to the loop end label.
- Otherwise, emit the body and jump back to the loop start label.

## Additional test
- at least one new test has been add. Compile and run all the tests and verify they all pass.