# Claude C99 Stage 3

Task: extend the compiler to process equality and relational expressions.

## Requirements:
Support the following new tokens: 
- == 
- != 
- < 
- <= 
- > 
- >=

The code for this project should be kept in a single c source file (compiler.c)

## Updated Grammar
```ebnf
<program>       ::= <function_decl>

<function_decl> ::= "int" <identifier> "(" ")" "{" <statement> "}"

<statement>     ::= "return" <expression> ";"

<expression> ::= <equality_expr>

<equality_expr> ::= <relational_expr> ( ("==" | "!=") <relational_expr> )*

<relational_expr> ::= <additive_expr> ( ( "<" | "<=" | ">" | ">=") <additive_expr> )*

<additive_expr> ::= <term> { ("+" | "-") <term> }*

<term>       ::= <factor> { ("*" | "/") <factor> }*

<factor>     ::= <int_literal> | "(" <expression> ")"

<identifier>    ::= [a-zA-Z_][a-zA-Z0-9_]*

<int_literal> ::= [0-9]+
```
## Semantics
- Equality and relational operators evaluate to int.
- True is represented as 1.
- False is represented as 0.
- Operators at each grammar level are left-associate.
- Expressions such as 1 < 2 < 3 should be parsed according to the grammar 

## AST Updates
Add AST node support the binary operators which should use the AST_BINARY_OP type:
- ==
- !=
- <
- <=
- >
- >=
  
## Code Generation
Update code generation so equality and relational expression produce 0 or 1 as the final value in the result register.

## Additional test files to compile:
    - a number of additional c program files have already been added to the project.
    - the files names now have an expected result in the name. the format
        of the test file names for this stage will be test_<name>__<expected>.c 
        where <expected> will be an integer that should match the exit code
        when the test file is compile/linked than ran.
    - update the run_test.sh and run_tests.sh scripts to extract the <expected>
      values from the file name then use it to pass or fail the test after running
      the compiled version.
