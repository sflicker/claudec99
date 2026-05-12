```ebnf
# Claude C99 Grammar (Current)

<translation_unit> ::= <external_declaration> { <external_declaration> }

<external_declaration> ::= <function_definition>
                          | <declaration>

<function_definition>    ::= <declaration_specifiers> <declarator> <block_statement>

<declaration_specifiers> ::= <declaration_specifier> { <declaration_specifier> }
<declaration_specifier>  ::= <storage_class_specifier>
                            | <type_specifier>

<storage_class_specifier>   ::= "extern" | "static" | "typedef"

<parameter_list> ::= <parameter_declarator> { "," <parameter_declarator> }

<parameter_declarator> ::= <type_specifier> [ <declarator> ]

<block_statement> ::= "{" { <statement> } "}"

<statement> ::=  <declaration>
                    | <return_statement>
                    | <if_statement>
                    | <while_statement>
                    | <do_while_statement>
                    | <for_statement>
                    | <switch_statement>
                    | <labeled_statement>
                    | <block_statement>
                    | <jump_statement>
                    | <expression_statement>

<declaration> ::= <declaration_specifiers> <init_declarator_list> ";"

<init_declarator_list> ::= <init_declarator> { "," <init_declarator> }

<init_declarator> ::= <declarator> [ "=" <initializer> ]

<initializer> ::= <assignment_expression>
                | "{" <initializer_list> [ "," ] "}"

<initializer_list> ::= <initializer> { "," <initializer> }

<declarator> ::= { "*" } <direct_declarator>

<direct_declarator> ::= <identifier>
                       | "(" <declarator> ")"
                       | <direct_declarator> "[" [ <integer_literal> ] "]"
                       | <direct_declarator> "(" [ <parameter_list> ] ")"

<type_specifier> ::= <integer_type> 
                     | <typedef_name> 
                     | <enum_specifier> 
                     | <struct_specifier>

<typedef_name> ::= <identifier>

<type_name> ::= <type_specifier> { "*" }

<integer_type> ::= "char" | "short" | "int" | "long"

<enum_specifier> ::= "enum" <identifier> "{" <enumerator_list> "}"
                   | "enum"             "{" <enumerator_list> "}"
                   | "enum" <identifier>

<enumerator_list> ::= <enumerator> { "," <enumerator> } [","]

<enumerator> ::= <identifier> [ "=" <constant_expression> ]

<struct_specifier> ::= "struct" <identifier> "{" <struct_declaration_list> "}"
                     | "struct" <identifier>

<struct_declaration_list> ::= <struct_declaration> { <struct_declaration> }

<struct_declaration> ::= <type_specifier> <struct_declarator_list> ";"

<struct_declarator_list> ::= <declarator> { "," <declarator> }

<return_statement> ::= "return" <expression> ";"

<if_statement> ::= "if" "(" <expression> ")" <statement> [ "else" <statement> ]
 
<while_statement> ::= "while" "(" <expression> ")" <statement>

<do_while_statement> ::= "do" <statement> "while" "(" <expression> ")" ";"

<for_statement> ::= "for" "(" [<expression>] ";" [<expression>] ";" [<expression>] ")" <statement>

<switch_statement> ::= "switch" "(" <expression> ")" <statement>

<labeled_statement> ::= <identifier> ":" <statement>
                      | "case" <constant_expression> ":" <statement>
                      | "default" ":" <statement>

<constant_expression> ::= <integer_literal> | <character_literal>

<jump_statement> ::= "continue" ";"
                    | "break" ";"
                    | "goto" <identifier> ";"

<expression_statement> ::= <expression> ";"

<expression> ::= <assignment_expression> { "," <assignment_expression> }

<assignment_expression> ::= <unary_expression> <assignment_operator> <assignment_expression>
                           | <conditional_expression>
                           
<assignment_operator> ::= "=" | "+=" | "-=" | "*=" | "/=" | "%=" |
                          "<<=" | ">>=" | "&=" | "^=" | "|="

<conditional_expression> ::= <logical_or_expression>
                           | <logical_or_expression> "?" <expression> ":" <conditional_expression>
 
<logical_or_expression> ::= <logical_and_expression> { "||" <logical_and_expression> }
 
<logical_and_expression> ::= <bitwise_or_expression> { "&&" <bitwise_or_expression> }

<bitwise_or_expression> ::= <bitwise_xor_expression> { "|" <bitwise_xor_expression> }

<bitwise_xor_expression> ::= <bitwise_and_expression> { "^" <bitwise_and_expression> }

<bitwise_and_expression> ::= <equality_expression> { "&" <equality_expression> }

<equality_expression> ::= <relational_expression> { ("==" | "!=") <relational_expression> }

<relational_expression> ::= <shift_expression> { ( "<" | "<=" | ">" | ">=") <shift_expression> }

<shift_expression> ::= <additive_expression> { ( "<<" | ">>" ) <additive_expression> }

<additive_expression> ::= <multiplicative_expression> { ("+" | "-") <multiplicative_expression> }

<multiplicative_expression> ::= <cast_expression> { ("*" | "/" | "%") <cast_expression> }

<cast_expression> ::= "(" <type_name> ")" <cast_expression>
                    | <unary_expression>

<unary_expression> ::= <sizeof_expression>
                    | <unary_operator> <unary_expression>
                    | <postfix_expression>

<sizeof_expression> ::= "sizeof" <unary_expression>
                      | "sizeof" "(" <type_name> ")"

<unary_operator> ::= "+" | "-" | "!" | "~" | "++" | "--" | "*" | "&"
                    
<postfix_expression> ::= <primary_expression> 
                    { "[" <expression> "]"                     
                      | "++" 
                      | "--"
                      | "(" [ <argument_expression_list> ] ")"
                      | "." <identifier>
                      | "->" <identifier> }                    

<primary_expression> ::= <integer_literal>
                         | <string_literal>
                         | <character_literal>
                         | <identifier>
                         | "(" <expression> ")"

<argument_expression_list> ::= <assignment_expression> { "," <assignment_expression> }

<identifier> ::= [a-zA-Z_][a-zA-Z0-9_]*

<integer_literal> ::= [0-9]+ [Ll]?

<string_literal> ::= TOKEN_STRING_LITERAL

<character_literal> ::= TOKEN_CHAR_LITERAL

# Lexical form of a character literal:
#   "'" ( <ordinary_char> | <character_escape_sequence> ) "'"
# A character constant has type int.

<string_char> ::= <ordinary_char>
                | <escape_sequence>

<escape_sequence> ::= "\n" | "\t" | "\r" | "\\" | "\"" | "\0"

<character_escape_sequence> ::= "\a" | "\b" | "\f" | "\n" | "\r" | "\t" | "\v"
                              | "\\" | "\'" | "\"" | "\?" | "\0"

# Current Restrictions:
#
# Declarations:
#   - In a function definition, the declarator must be a function declarator.
#   - Function declarations may not have initializers.
#   - extern/static storage-class specifiers are currently supported only
#     for file-scope declarations.
#   - extern declarations with initializers are currently rejected.
#   - Function definition parameters must be named; unnamed parameters are
#     only allowed in function prototypes and function pointer signatures.
#
# Typedefs:
#   - Scalar integer typedefs (char, short, int, long) are supported.
#   - Pointer typedefs are supported (e.g. typedef int *IntPtr;).
#   - Function pointer typedefs are supported (e.g. typedef int (*BinaryFn)(int, int);).
#   - Array typedefs are supported (e.g. typedef int A[4];).
#   - Chained typedefs (typedef of a typedef) are supported.
#   - typedef declarations may not have initializers.
#   - Non-pointer function typedefs, struct typedefs, and typedef enum are not yet supported.
#   - <typedef_name> is a semantic rule: an identifier is a typedef_name only
#     if it is currently known as a typedef in the active scope.
#
# Enums:
#   - Enum constants have flat (non-scoped) visibility regardless of where the
#     enum is declared.
#   - Explicit enumerator values must be integer or character literals only.
#     Full constant expressions (e.g. 1+2) are not yet supported.
#   - typedef enum is not yet supported.
#   - Forward enum declarations without a body are not supported.
#
# Arrays:
#   - Omitted array size is only supported for block-scope char arrays
#     initialized from string literal.
#   - Block-scope array initializers support brace-enclosed lists (stage 32)
#     with partial initialization zero-filling remaining elements.
#   - File-scope array initializers are not currently supported.
#   - Array declarators in multi-declarator lists are not currently supported.
#
# Initializers:
#   - File-scope object initializers must currently be integer or character
#     literals. Full constant expressions are not yet supported.
#   - Struct initializers support brace-enclosed lists (stage 32) and
#     whole-struct copy from a variable of the same type (stage 33).
#   - Struct assignment (`d = c`) copies all bytes when both operands share
#     the same struct type; mismatched struct types are rejected (stage 33).
#
# Statements:
#   - for-loop initializers are expressions only, not declarations.
#
# Function pointers:
#   - Nested function-pointer parameters (function pointer taking function
#     pointer) are not yet supported.
#   - Functions returning function pointers are not yet supported.
#   - Pointer-to-pointer-to-function is not yet supported.
#   - Arrays of function pointers are not yet supported.
#
# Semantics:
#   - Assignment left-hand sides must be valid lvalues.
#
# Structs:
#   - Struct-by-value function parameters are not yet supported.
#     Pointer-to-struct parameters (struct T *p) are supported (stage 34).
#   - Struct-by-value return types are not yet supported.

```
