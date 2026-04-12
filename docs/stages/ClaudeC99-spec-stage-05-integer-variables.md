# Claude C99 Stage 5

Task: extend the compiler to support variables.

## Requirements:

### Support the following new tokens:
- `=`

A single `=` character is for assignment. 

The tokenizer must distinguish between`=` and the existing multi-character operators:
- `==`
- `!=`
- `<=`
- `>=`

### Updated Grammar

This stage adds variable declarations and assignments.

```ebnf
<program> ::= <function>

<function> ::= "int" <identifier> "(" ")" <block_statement>

<block_statement> ::= "{" { <statement> } "}"

 <statement> ::=  <declaration>
                    | <assignment_statement>
                    | <return_statement>
                    | <if_statement>
                    | <block_statement>
                    | <expression_statement>

  <declaration> ::= "int" <identifier> [ "=" <expression> ] ";"

 <assignment_statement> ::= <identifier> "=" <expression> ";"
 
 <return_statement>     ::= "return" <expression> ";"

 <if_statement> ::= "if" "(" <expression> ")" <statement> [ "else" <statement> ]

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
- Variables are introduced only through <declaration>. 
- Only 'int' variables will be allowed for this stage.
- Variables must be declared before being used.
- For this stage variables are only allowed as a child of a functions outermost scope.- 
- variables may be declared anywhere within this scope.
- No global or nested scope declarations for this stage.

- All variables belong to one function-level local scope.
- Using an undeclared variable is a compile-time error.
- Initialization in a declaration is optional.
- Assignment is a statement only in this stage, not an expression.
- If the compiler finds a variable that was not first declared this will be an error and the error message should at least contain "undeclared variable" along with the variable name.
- If the compiler finds a variable declaration that was already declared this will be an error and the error message should at least contain "duplicate declaration" along with the variable name.

## Code Generation Expectations
- Each local variable should be stored in stack space.
- The compiler should maintain a per-function local symbol table mapping variable names to stack offsets.
- A declaration should allocate stack space for the variable.
- An initialized declaration such as int x = 5; should both allocate storage and store the initial value.
- An assignment such as x = 7 should evaluate the right-hand side and store the result into the variable's stack slot.
- A variable used in an expression should load its value from the stack slot.

## Errors to detect
At a minimum the compiler should report an error for:
- use of an undeclared variable
- assignment to an undeclared variable
- duplicate declaration of the same variable in the same scope.
- For this stage a variable declared somewhere besides a child of a function's outer most scope should be an error.

## Additional tests
- A number of additional c program files have already been added to test the new features of this stage. All tests should pass after the implementation of this spec.
- The existing tests have been moved from the folder test -> test/valid.
- A new type of test file has been added to a folder test/invalid. These are test files that contain errors. 
   The compiler should attempt to compile these and the expected behavior is to fail and output a error message.
   The error message should be specific to the type of error encountered. The test file name will have the format
   test_<name>__<expected error message fragment>.c. For the test to pass an error message should be generated that
   contains the expected message fragment.