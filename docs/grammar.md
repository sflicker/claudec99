```ebnf
# Claude C99 Grammar (Current)

<translation-unit> ::= <external-declaration> { <external-declaration> }

<external_declaration> ::= <function_definition>
                          | <declaration>

<function_definition>    ::= <declaration_specifiers> <declarator> <block_statement>

<declaration_specifiers>    ::= [ <storage_class_specifier> ] <type_specifier>

<storage_class_specifier>   ::= "extern" | "static"

# Restriction: the declarator in <function_definition> must be a function declarator.
# Restriction: function declarations at file scope may not have an initializer.
# Restriction: storage class specifiers are not allowed at block scope.
# Restriction: extern declarations may not have an initializer.
# Restriction: only one storage class specifier is allowed per declaration.

<parameter_list> ::= <parameter_declaration> { "," <parameter_declaration> }

<parameter_declaration> ::= <type_specifier> <declarator>
                        | <type_specifier> <func_ptr_declarator>

<func_ptr_declarator> ::= { "*" } "(" "*" <identifier> ")" "(" [ <func_ptr_param_list> ] ")"

<func_ptr_param_list> ::= <func_ptr_param> { "," <func_ptr_param> }

<func_ptr_param> ::= <type_specifier> { "*" } [ <identifier> ]

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

# Restriction: an omitted array size is only allowed when the
# declaration has a string-literal initializer. The string-literal
# initializer form is only allowed when the element type is `char`.
# Restriction: array declarators are not supported in multi-declarator lists.

<init_declarator> ::= <declarator> [ "=" <initializer_expression> ]

<initializer_expression> ::= <assignment_expression>

<declarator> ::= { "*" } <direct_declarator>

<direct_declarator> ::= <identifier>
                       | "(" <declarator> ")"
                       | <identifier> "[" [ <integer_literal> ] "]"
                       | <identifier> "(" [ <parameter_list> ] ")"

<type_specifier> ::= <integer_type>

<type_name> ::= <type_specifier> { "*" }

<integer_type> ::= "char" | "short" | "int" | "long"

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
                      | "--" }                    

<primary_expression> ::= <integer_literal>
                         | <string_literal>
                         | <character_literal>
                         | <function_call>
                         | <identifier>
                         | "(" <expression> ")"

<function_call> ::= <identifier> "(" [ <argument-expression-list> ] ")"

<argument-expression-list> ::= <assignment_expression> { "," <assignment_expression> }

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

# Restriction : file-scope object initializers must be a constant primary expression
#   (integer literal or character literal). Full assignment expressions are not allowed.
# Restriction : file-scope array declarations may not have an initializer.
# Current Restriction : for-loop initializers are expressions only, not declarations
# Current Restriction : array declarations are limited to a single bracket suffix.
# Semantic Restriction : assignment left-hand sides must be valid lvalues.

# Restriction: <func_ptr_declarator> allows pointer-to-function declarations only.
#   Pointer-to-pointer-to-function (e.g., int (**fp)(int)) and function-returning-pointer-to-function
#   (e.g., int (*(*factory())(int))(int)) are not supported.
# Restriction: calls through function pointers are not supported in this stage.
#   Assignment to function pointer variables from function designators (stage 25-02) is supported.

```
