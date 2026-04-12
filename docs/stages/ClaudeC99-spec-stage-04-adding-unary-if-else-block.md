# Claude C99 Stage 4

Task: extend the compiler to support unary expressions, block statements, 
      expression statements and if/else control flow.

## Requirements:

### Support the following new tokens:
- if
- else
- !
- identifier

identifier - a sequence of letters and digits. The first character must be a letter. 
    Underscores count as letters. Identifiers will follow this rule:
    [a-zA-Z_][a-zA-Z0-9_]* 

### Updated Grammar
```ebnf
<program>       ::= <function>

 <function> ::= "int" <identifier> "(" ")" <block>

 <block> ::= "{" { <statement> } "}"

 <statement> ::= <return_statement>
                    | <if_statement>
                    | <block>
                    | <expression_stmt>

 <return_statement>     ::= "return" <expression> ";"

 <if_statement> ::= "if" "(" <expression> ")" <statement> [ "else" <statement> ]

 <expression_stmt> ::= <expression> ";"

 <expression> ::= <equality_expr>

 <equality_expr> ::= <relational_expr> { ("==" | "!=") <relational_expr> }

 <relational_expr> ::= <additive_expr> [ ( "<" | "<=" | ">" | ">=") <additive_expr> ]*

 <additive_expr> ::= <term> [ ("+" | "-") <term> ]*

 <term>       ::= <unary_expr> [ ("*" | "/") <unary_expr> ]*
   
 <unary_expr> ::= [ "+" | "-" | "!" ] <unary_expr> | <primary>

 <primary>     ::= <int_literal> 
                    | "(" <expression> ")"

 <identifier>    ::= [a-zA-Z_][a-zA-Z0-9_]*

 <int_literal> ::= [0-9]+

```

## Semantics
 - For this stage identifier's should be tokenized and parsed but are only used for function names at this stage.
 - if conditions use integer truthiness: 0 is false, nonzero is true.
 - Unary ! returns 1 if operand is zero, otherwise 0.
 - Unary - negates the operand.
 - Unary + leaves the operand unchanged.
 - else binds to the nearest unmatched if.

## AST Updates
- add a new AST to support Unary expression. Each Unary AST should include an operator token and a child expression.

## Additional tests
- A number of additional c program files have already been added to test the new features of this stage.