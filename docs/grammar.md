```ebnf
# Claude C99 Grammar (Current through Stage 70)
#
# Note: Stage 70 adds an integration test (test_mini_compiler) that exercises
# existing language features in a realistic tokenizer pattern. No grammar changes.
#


<translation_unit> ::= <external_declaration> { <external_declaration> }

<external_declaration> ::= <function_definition>
                          | <declaration>

<function_definition>    ::= <declaration_specifiers> <declarator> <block_statement>

<declaration_specifiers> ::= <declaration_specifier> { <declaration_specifier> }

<declaration_specifier>  ::= <storage_class_specifier>
                            | <type_specifier>
                            | <sign_specifier>
                            | <type_qualifier>

<sign_specifier> ::= "signed" | "unsigned"

<type_qualifier> ::= "const"

<storage_class_specifier>   ::= "extern" | "static" | "typedef"

<parameter_list> ::= "void"
                   | <parameter_declarator> { "," <parameter_declarator> } [ "," "..." ]

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

<type_specifier> ::= "void"
                     | <integer_type_specifier> 
                     | "_Bool"
                     | <typedef_name> 
                     | <enum_specifier> 
                     | <struct_specifier>

<typedef_name> ::= <identifier>

<type_name> ::= <type_specifier> { "*" }

<integer_type_specifier> ::= "char"
                           | "short" [ "int" ]
                           | "int"
                           | "long" [ "int" ]
                           | "long" "long" [ "int" ]

<integer_suffix> ::= [Uu]
                   | [Ll]
                   | "ll" | "LL"
                   | [Uu][Ll]
                   | [Ll][Uu]
                   | [Uu]("ll"|"LL")
                   | ("ll"|"LL")[Uu]

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

<return_statement> ::= "return" [ <expression> ] ";"

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

<integer_literal> ::= [0-9]+ [ <integer_suffix> ]

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

# Preprocessor Directives (preprocessing phase, before parsing):

<preprocessor_directive> ::= <object_like_macro_directive>
                           | <function_like_macro_directive>
                           | <include_directive>
                           | <undef_directive>
                           | <conditional_directive>

<object_like_macro_directive> ::= "#" "define" <identifier> [ <replacement_list> ]

<function_like_macro_directive> ::= "#" "define" <identifier> "(" [ <parameter_list> ] ")" <replacement_list>

<include_directive> ::= "#" "include" "<filename>"
                      | "#" "include" "\"" <filename> "\""

<undef_directive> ::= "#" "undef" <identifier>

<conditional_directive> ::= <ifdef_directive>
                          | <ifndef_directive>
                          | <if_constant_directive>
                          | <elif_constant_directive>
                          | <else_directive>
                          | <endif_directive>

<ifdef_directive> ::= "#" "ifdef" <identifier>

<ifndef_directive> ::= "#" "ifndef" <identifier>

<if_constant_directive> ::= "#" "if" <if-condition>

<elif_constant_directive> ::= "#" "elif" <if-condition>

<if-condition> ::= <pp-logical-or-expression>

<pp-logical-or-expression> ::= <pp-logical-and-expression>
                              | <pp-logical-or-expression> "||" <pp-logical-and-expression>

<pp-logical-and-expression> ::= <pp-bitwise-or-expression>
                               | <pp-logical-and-expression> "&&" <pp-bitwise-or-expression>

<pp-bitwise-or-expression> ::= <pp-bitwise-xor-expression>
                              | <pp-bitwise-or-expression> "|" <pp-bitwise-xor-expression>

<pp-bitwise-xor-expression> ::= <pp-bitwise-and-expression>
                               | <pp-bitwise-xor-expression> "^" <pp-bitwise-and-expression>

<pp-bitwise-and-expression> ::= <pp-equality-expression>
                               | <pp-bitwise-and-expression> "&" <pp-equality-expression>

<pp-equality-expression> ::= <pp-relational-expression>
                            | <pp-equality-expression> "==" <pp-relational-expression>
                            | <pp-equality-expression> "!=" <pp-relational-expression>

<pp-relational-expression> ::= <pp-shift-expression>
                              | <pp-relational-expression> "<"  <pp-shift-expression>
                              | <pp-relational-expression> "<=" <pp-shift-expression>
                              | <pp-relational-expression> ">"  <pp-shift-expression>
                              | <pp-relational-expression> ">=" <pp-shift-expression>

<pp-shift-expression> ::= <pp-additive-expression>
                         | <pp-shift-expression> "<<" <pp-additive-expression>
                         | <pp-shift-expression> ">>" <pp-additive-expression>

<pp-additive-expression> ::= <pp-multiplicative-expression>
                            | <pp-additive-expression> "+" <pp-multiplicative-expression>
                            | <pp-additive-expression> "-" <pp-multiplicative-expression>

<pp-multiplicative-expression> ::= <pp-unary-expression>
                                  | <pp-multiplicative-expression> "*" <pp-unary-expression>
                                  | <pp-multiplicative-expression> "/" <pp-unary-expression>
                                  | <pp-multiplicative-expression> "%" <pp-unary-expression>

<pp-unary-expression> ::= <pp-primary>
                         | <pp-unary-op> <pp-unary-expression>

<pp-unary-op> ::= "!" | "-" | "+" | "~"

<pp-primary> ::= <integer-literal>
               | "defined" "(" <identifier> ")"
               | "defined" <identifier>
               | <object-like-macro-identifier>
               | "(" <pp-logical-or-expression> ")"

<else_directive> ::= "#" "else"

<endif_directive> ::= "#" "endif"

<replacement_list> ::= any sequence of tokens

<parameter_list> ::= <identifier> { "," <identifier> }

# Current Restrictions:
#
# Declarations:
#   - In a function definition, the declarator must be a function declarator.
#   - Function declarations may not have initializers.
#   - The `extern` storage-class specifier is supported only at file scope.
#   - The `static` specifier is supported at file scope and block scope
#     (scalar and pointer types only; block-scope static arrays and structs
#     are not yet supported).
#   - extern declarations with initializers are currently rejected.
#   - Function definition parameters must be named; unnamed parameters are
#     only allowed in function prototypes and function pointer signatures.
#
# Typedefs:
#   - Scalar integer typedefs (char, short, int, long, unsigned char, unsigned short, unsigned int, unsigned long) are supported.
#   - Pointer typedefs are supported (e.g. typedef int *IntPtr;).
#   - Function pointer typedefs are supported (e.g. typedef int (*BinaryFn)(int, int);).
#   - Array typedefs are supported (e.g. typedef int A[4];).
#   - Chained typedefs (typedef of a typedef) are supported.
#   - typedef declarations may not have initializers.
#   - Struct typedefs are supported (stage 36): both `typedef struct Tag Alias;` and
#     `typedef struct Tag { ... } Alias;` forms.
#   - Non-pointer function typedefs and typedef enum are not yet supported.
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
#   - Omitted array size is supported for arrays initialized from a string literal
#     (char arrays only) or a brace-enclosed initializer list at both block scope and file scope.
#   - Block-scope array initializers support brace-enclosed lists (stage 32)
#     with partial initialization zero-filling remaining elements.
#   - Array declarators in multi-declarator lists are not currently supported.
#   - Too-many-elements errors are now reported for explicitly-sized arrays with excess initializers
#     (both local and file scope).
#
# Initializers:
#   - File-scope object initializers support: integer and character literals (scalar types),
#     string literals (pointer types, char *s = "abc"), and brace-enclosed constant lists
#     for arrays (char s[] = "abc", int a[] = {1,2,3}, char *names[] = {"a","b"}). 
#     Full constant expressions are not yet supported.
#   - Struct initializers support brace-enclosed lists (stage 32) and
#     whole-struct copy from a variable of the same type (stage 33).
#   - Struct aggregate initializers (brace-enclosed lists) are now supported at both block scope
#     and file scope (stage 44), including nested aggregate initializers for struct-typed fields.
#     Type checking: string literals are rejected for non-pointer fields; non-null integers are
#     rejected for pointer fields.
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
#   - Incomplete struct declarations (forward declarations) are supported (stage 37):
#     `struct Tag;` can appear before the body is defined, enabling self-referential
#     structs via typedef aliases and opaque pointer fields.
#   - Non-pointer incomplete struct fields are rejected; only pointer-to-incomplete-struct
#     fields are allowed as struct members.
#   - Struct-by-value function parameters are not yet supported.
#     Pointer-to-struct parameters (struct T *p) are supported (stage 34).
#   - Struct-by-value return types are not yet supported.
#
# void type (stage 38):
#   - void cannot be used as an object type (void x; is rejected at all scopes).
#   - void * is a generic object pointer compatible with any non-function pointer type.
#   - Dereferencing void * is rejected.
#   - Pointer arithmetic on void * is rejected.
#   - Subscript on void * is rejected.
#   - sizeof(void) is rejected.
#   - The f(void) parameter list means zero parameters.
#   - Functions returning void do not need a return statement; they may use bare return;
#     or fall off the end.
#
# const qualifier (stage 39):
#   - const qualifiers are parsed and stored on base types.
#   - Assignment to simple const-qualified variables (const int x; x = 5;) is rejected.
#   - Pointer-level const enforcement is not yet supported: writes through const pointers
#     (const int *p; *p = x;), const-to-non-const conversions, and pointer-to-const
#     const-correctness checks are deferred to a later stage.
#
# Pointer arithmetic (stage 41):
#   - p + n and p - n: address(p) ± n * sizeof(*p); result type is T*.
#   - n + p: equivalent to p + n.
#   - p += n and p -= n: compound assignment forms; scale by sizeof(*p).
#   - ++p, p++, --p, p--: prefix/postfix increment/decrement; step is sizeof(*p).
#   - p1 - p2: pointer difference; result is (address(p1) - address(p2)) / sizeof(*p1),
#     a signed long (ptrdiff_t equivalent). Requires identical pointee types.
#   - p[n]: subscript; equivalent to *(p + n). Supported for both arrays and pointers.
#   - Pointer arithmetic on void * is rejected.
#   - Pointer arithmetic on function pointers is rejected.
#
# Preprocessing (stages 50–55):
#   - Object-like `#define NAME replacement-list` defines a macro that expands
#     to the replacement text whenever the macro name appears as an identifier
#     in ordinary source text outside string and character literals.
#   - Empty replacement macros are supported (e.g., `#define EMPTY`).
#   - Macros are stored in a table shared across all preprocessing passes,
#     so macros defined in `#include "header.h"` are visible to the including file.
#   - Compatible redefinition (`#define NAME` with the same replacement text)
#     is allowed silently.
#   - Incompatible redefinition (`#define NAME` with different replacement text)
#     produces a fatal error containing "incompatible replacement".
#   - Macro expansion outputs the replacement text verbatim with no re-scanning
#     or token re-parsing.
#   - Conditional directives (`#ifdef`, `#ifndef`, `#else`, `#endif`) control whether
#     source code regions are included in the preprocessed output. The condition is based
#     on whether a macro name is currently defined. `#elif <integer>` provides additional
#     branches with first-true-wins semantics. Inactive regions are suppressed from
#     output; directives and macro definitions in inactive regions are not processed.
#     Nesting is supported up to depth 64. Errors are produced for missing `#endif`,
#     duplicate `#else`, unmatched directives, and `#elif` without a conditional.
#   - `#if` and `#elif` directives support unary operators `!`, `-`, and `+`, binary
#     equality and relational operators (`==`, `!=`, `<`, `<=`, `>`, `>=`), and logical
#     operators `&&` and `||` with C-like precedence (`&&` binds tighter than `||`, both
#     lower than relational/equality). Conditions may include integer literals (e.g., `#if 1`),
#     the `defined()` operator with or without parentheses (e.g., `#if defined(NAME)` or
#     `#if defined NAME`), object-like macro identifiers that expand to integer literals
#     (e.g., `#define DEBUG 1; #if DEBUG`), and combinations thereof
#     (e.g., `#if VERSION >= 2 && ENABLED`, `#if defined(A) || defined(B)`).
#     The condition value is determined as: nonzero or defined = true, zero or undefined = false;
#     `&&` and `||` produce 0 or 1.
#   - Function-like macros (`#define NAME(...)`), nested function-like macro invocations,
#     and stringification (`#param` in function-like macro replacement lists) are supported (stage 57).
#   - Token pasting (`##`) in function-like macro replacement lists is supported.
#   - Variadic function-like macros (`#define M(...)`, `#define M(x, ...)`) with
#     `__VA_ARGS__` expansion are supported.
#   - `#if`/`#elif` conditions support bitwise (`~`, `&`, `^`, `|`) and shift
#     (`<<`, `>>`) operators (see the pp-expression grammar above).
#   - Recursive macro expansion beyond simple guarding and `#elifdef`/`#elifndef`
#     are not yet supported.

```
