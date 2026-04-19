# Claude C99 Stagea 09-04

## Task: extend the compiler to support function calls.

## Requirements:
- Allow functions to be called as expressions.
- A function call may appear anywhere an expression is allowed.
- The value of a function call expression is the `int` value returned by the called function.
- The called function must already be defined.
- A function call must match an existing function definition by name and argument count.
- A function call is an expression and return values are used the same as values from other expressions.
- If a function call name does not match an existing function name a compile error results.
- If a function call does not match the arguments of the existing function a compile error results.
- For this stage all functions have an int return type
- For this stage all functions must return an int value.
- For this stage all function definition parameters must be int
- For this stage all function call arguments must be int
- For this stage function declaration (prototypes) are not allowed.
- A function may call itself from within its own body.
- Function calls must use the System V ABI AMD64 conventions.
- For this stage function definitions and function calls may have at most 6 arguments. This should simplify the
   code generation as all arguments will be passed only through registers instead of also using the stack when
   more than 6.
- Argument expressions are evaluated left to right.
- Function call expressions must be limited to identifier only
  - `foo(1,2)` is allowed
  - `(foo)(1,2)` is NOT allowed
  - function pointers are not allowed

## Notes for this stage
- The first 6 integer arguments as passed in:
  - rdi
  - rsi
  - rdx
  - rcx
  - r8
  - r9
- The return value of a function is placed in rax.

## Updated Grammar:

```ebnf

<translation-unit> ::= <external-declaration> { <external-declaration> }

<external-declaration> ::= <function>

<function> ::= "int" <identifier> "(" [ <parameter_list> ] ")" <block_statement>

<parameter_list> ::= <parameter_declaration> { "," <parameter_declaration> }

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
                         | <function_call>
                         | "(" <expression> ")"

<function_call> ::= <identifier> "(" [ <argument-expression-list> ] ")"

<argument-expression-list> ::= <assignment_expression> { "," <assignment_expression> }

<identifier> ::= [a-zA-Z_][a-zA-Z0-9_]*

<int_literal> ::= [0-9]+
```
