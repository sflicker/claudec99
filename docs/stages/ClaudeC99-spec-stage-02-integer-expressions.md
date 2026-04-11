# Claude C99 Stage 2

Task: extend the minimal compiler to process basic integer expressions.

Requirements:
Supports the following new tokens: + - * / ( )
New Token types should be added for each

Updated Grammar:

   <program>       ::= <function_decl>

   <function_decl> ::= "int" <identifier> "(" ")" "{" <statement> "}"

   <statement>     ::= "return" <expression> ";"

   <expression> ::= <term> { ("+" | "-") <term> }*

   <term>       ::= <factor> { ("*" | "/") <factor> }*
   
   <factor>     ::= <int_literal> | "(" <expression> ")"

   <identifier>    ::= [a-zA-Z_][a-zA-Z0-9_]*

    <int_literal> ::= [0-9]+
    
The operators +, -, * and / are left associative. Also for this stages these are
all binary operators. Do not implement the unary forms at this stage.

For integers only 0 and positive will be handled in this stage.

For division only normal integer division will be handled in this stage.

update to Token data structure:
include an int_value field that will hold the integer value for int_literal and 0 for other token types

Update to AST:
A new AST type for binary op's needs to be added. Each Binary AST node should store a operator, a left child and a right child.


Additional test files to compile:
a number of additional c program files have already been added. These all perform basic integer operations and all should result in 42 which is output as the exit code similar to the one from stage 1. Make a script to run all of these  tests and verify they pass. 
- The script should compile each .c test input file.
- assemble and link
- run the resulting executable
- compare the exit code with the expected value. For this stage all the expected values will be 42.
- print pass or fail per test and provide a summary at the end.
