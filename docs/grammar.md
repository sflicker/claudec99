```ebnf
# Claude C99 Grammar (Current)

<translation_unit> ::= <external_declaration> { <external_declaration> }

<external_declaration> ::= <function_definition>
                          | <declaration>

<function_definition>    ::= <declaration_specifiers> <declarator> <block_statement>

<declaration_specifiers>    ::= [ <storage_class_specifier> ] <type_specifier>

<storage_class_specifier>   ::= "extern" | "static"

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

<init_declarator> ::= <declarator> [ "=" <initializer_expression> ]

<initializer_expression> ::= <assignment_expression>

<declarator> ::= { "*" } <direct_declarator>

# Current restriction: array declarators are limited to a single bracket suffix.

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
                      | "--"
                      | "(" [ <argument_expression_list> ] ")" }                    

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
#
# Arrays:
#   - Array declarations are currently limited to a single bracket suffix.
#   - Omitted array size is only supported for block-scope char arrays 
#      initialized from string literal.
#   - File-scope array initializers are not currently supported.
#   - Array declarators in multi-declarator lists are not currently supported.
#
# Initializers:
#   - File-scope object initializers must currently be integer or character
#     literals. Full constant expressions are not yet supported.
#
# Statements:
#   - for-loop initializers are expressions only, not declarations.
#
# Function pointers:
#   - Function pointer declarations are still handled by a special
#     <func_ptr_declarator> path.
#   - Fully general recursive function declarators are not yet supported.
#
# Semantics:
#   - Assignment left-hand sides must be valid lvalues.

```
