# Claude C99 Stage 7.3

Task: Extend the compiler to support increment and decrement operators.

## Requirements:

### Support the following new tokens:
- `++`
- `--`

### Updated Grammar

This stage adds support for increment and decrement operators

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
   
<unary_expression> ::= [ "+" | "-" | "!" | "++" | "--" ] <unary_expression>  
                    | "++" <identifier>
                    | "--" <identifier>
                    | <postfix_expression>
                    
<postfix_expression> ::= <primary_expression> { "++" | "--" }                    

<primary_expression> ::= <int_literal> 
                         | <identifier>
                         | "(" <expression> ")"

<identifier> ::= [a-zA-Z_][a-zA-Z0-9_]*

<int_literal> ::= [0-9]+
```
## Semantics

This stage adds prefix and postfix increment/decrement for integer variables.

### Prefix Operators 
- `++x` 
  - increments `x` by 1 
  - expression evaluates to the new value.
- `--x`
  - decrements `x` by 1 
  - expression evaluates to the new value.

### Postfix Operators
- `x++`
  - expression evaluates to the original value of `x`
  - then increments `x` by 1.
- `x--`
  - expression evaluates to the original value of `x`
  - then decrements `x` by 1.

### Scope and Restrictions
To keep this stage focused and simple: 
- increment and decrement operators only apply to identifiers.
- only `int` variables are supported.
- do not implement pointer or complex lvalue semantics
- do not add general lvalue analysis beyond what is needed for identifiers.

### Invalid forms for this stage

- `++5`
- `(a+b)++`
- `--(x+1)`

### Supported Usage
This following forms must work correctly:

#### Standalone expression statements
- `i++;`
- `++i;`
- `i--;`
- `--i;`

#### Within expressions
- `x = i++;`
- `x = ++i;`
- `y = a + b++;`
- `z = --i * 2;`
- `return i++;`

#### In loop expressions
- `for (i = 0;i < 10; i++) {....}`

#### Notes for Implementation
- Prefix operators are handled in `<unary_expressions>`
- Postfix operators are handled in `<postfix_expressions>`
- Postfix operators bind tighter than prefix operators
- Ensure correct evaluation order:
  - Postfix: return old value then update
  - Prefix: update first, then return new value

### Ouf of Scope (Future Stages)
The following are intentionally not required for this state:
- Chained increments like `i++++`
- Increment on non-identifiers (true lvalue analysis)
- Pointer increment/decrement
- Floating point types
- Undefined behavior detection