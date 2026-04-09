Task: Implement a very minimal c compiler.

Requirements:
- Language: c
- Recursive descent parser
- Program will accept a single text file containing a c program
- Program will consist of a single file
- Program output will be x86_64 assembly language in intel format suitable for input to nasm.
- This is the first stage later stages will add on.
- The program should first read the input and convert it to tokens
- Next the program should convert the tokens into an Abstract Syntax Tree
- Next the program should use the Syntax Tree to generate the assembly language output and write it to a file.
- For building a CMakeFile should be used

Here are the tokens to support for this stage:
	TOKEN_EOF,
    TOKEN_INT,
    TOKEN_RETURN,
    TOKEN_IDENTIFIER,
    TOKEN_INT_LITERAL,
    TOKEN_LPAREN,
    TOKEN_RPAREN,
    TOKEN_LBRACE,
    TOKEN_RBRACE,
    TOKEN_SEMICOLON,
    TOKEN_UNKNOWN

	
Here is the grammar for this stage
   <program>       ::= <function_decl>

   <function_decl> ::= "int" <identifier> "(" ")" "{" <statement> "}"

   <statement>     ::= "return" <expression> ";"

   <expression>    ::= <integer_literal>

   <identifier>    ::= [a-zA-Z_][a-zA-Z0-9_]*
    <integer_literal> ::= [0-9]+

