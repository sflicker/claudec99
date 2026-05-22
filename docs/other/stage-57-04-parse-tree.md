
# Full Grammar Parse Tree — Stage 57-04

Complete hierarchy of parser functions, grouped into four sections. Each level
calls the level(s) below it for sub-constructs. Indentation tracks call depth;
`└─►` indicates the primary descent path, `├─►` indicates a branch.

---

## Section 1 — Program Structure

```
parse_translation_unit
│  one or more external-declarations until EOF
│  AST_TRANSLATION_UNIT, children: [external_declaration+]
└─► parse_external_declaration
       dispatches on declaration_specifiers + declarator shape:
         typedef declaration        → AST_TYPEDEF_DECL
         function declaration       → AST_FUNCTION_DECL (no body)
         function definition        → AST_FUNCTION_DECL + AST_BLOCK
         file-scope object decl     → AST_DECLARATION (or AST_DECL_LIST)
         function-pointer object    → AST_DECLARATION (decl_type=TYPE_POINTER)
         standalone type-only decl  → AST_TYPEDEF_DECL (empty name)
       Variadic functions: is_variadic flag set on AST_FUNCTION_DECL
       void object declaration → compile-time error
    ├─► parse_declaration_specifiers
    │      Collects optional const qualifier + optional storage class + one type specifier.
    │      type qualifier: "const"
    │      storage class: "extern" | "static" | "typedef"
    │      type specifier: "void" | scalar | "unsigned" | "enum" | "struct" | typedef-name
    │      Returns DeclSpecResult {sc, base_kind, base_type, is_const}
    │   └─► parse_type_specifier
    │
    ├─► parse_declarator  [see Section 4]
    │
    ├─► parse_parameter_list  (when function declarator)
    │      { "," <parameter_declaration> }
    │      "..." at end of list: sets func->is_variadic = 1; stops parsing
    │   └─► parse_parameter_declaration  (×N)
    │          [ "const" ] <type_specifier> [ <declarator> | { "*" } ]
    │          unnamed scalar: next token is "," or ")"
    │          unnamed pointer: leading stars consumed, then "," or ")" → AST_PARAM
    │          AST_PARAM  (name in node->value, empty for unnamed)
    │          func-pointer param: decl_type=TYPE_POINTER, full_type=fp chain
    │       └─► parse_type_specifier
    │
    │      File-scope array declarations (non-function):
    │        has explicit size → TYPE_ARRAY; initializer optional
    │        omitted size + brace-initializer → size inferred from element count
    │        omitted size + string literal  → size inferred from string length+1
    │        too many initializers for explicit size → compile-time error
    │        struct global initializer: brace-list (AST_INITIALIZER_LIST child)
    │          non-struct with brace init → compile-time error
    │          non-pointer global string init → compile-time error
    │
    └─► parse_block  (when definition)
           "{" { <statement> } "}"
           AST_BLOCK, children: [statement*]
        └─► parse_statement  (×N)   [see Section 2]
```

---

## Section 2 — Statements

```
parse_statement
│  Dispatches on the current lookahead token:
│
├─► TOKEN_EXTERN | TOKEN_STATIC (block scope) → error
│
├─► "typedef"  →  typedef declaration at block scope
│      AST_TYPEDEF_DECL  (name in node->value)
│      forms accepted: scalar, pointer, array, function-pointer typedefs
│      unsigned scalar typedefs: full_type carries the unsigned base Type*
│      registers name in parser's typedef table with scope tracking
│   ├─► parse_type_specifier
│   └─► parse_declarator
│
├─► IDENTIFIER followed by ":"  →  labeled statement
│      AST_LABEL_STATEMENT  (label name in node->value)
│      children: [statement]
│      (lookahead is two tokens; lexer position is saved and restored on mismatch)
│   └─► parse_statement  (the labeled body)
│
├─► "const" | "void" | "char" | "short" | "int" | "long" | "unsigned" |
│   "enum" | "struct" | <typedef-name>
│      →  declaration (scalar, pointer, array, struct, enum, function-pointer)
│      [ "const" ] <type_specifier> <init_declarator_list> ";"
│
│      Leading "const": sets local_is_const; applies to variable when no pointer depth.
│      void object declaration (no pointer, no array, no func-pointer) → error
│
│   ├─► parse_type_specifier
│   ├─► parse_declarator  (×N, one per init-declarator in the list)
│   │      returns ParsedDeclarator {name, pointer_count, is_array, array_length,
│   │                               has_size, is_function, is_func_pointer, ...}
│   │
│   │  Standalone type-only declaration (no declarator, e.g. "struct S{};" ):
│   │      AST_TYPEDEF_DECL (empty name)
│   │
│   │  Single scalar/pointer declarator:
│   │      AST_DECLARATION  (name in node->value)
│   │      is_const=1 when const-qualified and no pointer depth
│   │      is_unsigned=1 when unsigned-typed scalar
│   │      optional initializer child: parse_initializer (allows brace-lists)
│   │      struct variable: decl_type=TYPE_STRUCT, full_type=Type*
│   │
│   │  Single array declarator:
│   │      AST_DECLARATION with decl_type=TYPE_ARRAY, full_type=type_array(...)
│   │      initializer options:
│   │        string-literal (char arrays only): AST_STRING_LITERAL child
│   │        brace-list: AST_INITIALIZER_LIST child
│   │        omitted size + brace-list: size inferred from element count
│   │        omitted size + string literal: size inferred from string length+1
│   │        omitted size without either initializer → error
│   │
│   │  Function-pointer declarator (*fp)(...):
│   │      AST_DECLARATION with decl_type=TYPE_POINTER, full_type=fp chain
│   │      optional initializer child: parse_assignment_expression
│   │
│   │  Multiple declarators (comma-separated; array/func-pointer forbidden):
│   │      AST_DECL_LIST, children: [AST_DECLARATION+]
│   │      each child may carry an optional initializer (parse_assignment_expression)
│   │      is_const propagated per-declarator based on pointer_count
│   │      is_unsigned propagated from base type
│   │      struct members in list: each child gets decl_type=TYPE_STRUCT
│   │
│   └─► parse_initializer  (initializer, when "=" present)  [see Section 3]
│
├─► "return"  →  return statement
│      "return" ";" (bare return)       → AST_RETURN_STATEMENT, no children
│      "return" <expression> ";"        → AST_RETURN_STATEMENT, children: [expression]
│   └─► parse_expression  (when not bare return)  [see Section 3]
│
├─► "if"  →  parse_if_statement
│      "if" "(" <expression> ")" <statement> [ "else" <statement> ]
│      AST_IF_STATEMENT, children: [condition, then_stmt, (opt) else_stmt]
│   ├─► parse_expression       (condition)
│   ├─► parse_statement        (then branch)
│   └─► parse_statement        (else branch, when TOKEN_ELSE present)
│
├─► "while"  →  parse_while_statement
│      "while" "(" <expression> ")" <statement>
│      AST_WHILE_STATEMENT, children: [condition, body]
│   ├─► parse_expression  (condition)
│   └─► parse_statement   (body, executed with loop_depth++)
│
├─► "do"  →  parse_do_while_statement
│      "do" <statement> "while" "(" <expression> ")" ";"
│      AST_DO_WHILE_STATEMENT, children: [body, condition]
│      Note: children order is body-first, condition-second (body parsed before condition)
│   ├─► parse_statement   (body, executed with loop_depth++)
│   └─► parse_expression  (condition)
│
├─► "for"  →  parse_for_statement
│      "for" "(" [<expr>] ";" [<expr>] ";" [<expr>] ")" <statement>
│      AST_FOR_STATEMENT, children[0..3]: [init|NULL, cond|NULL, update|NULL, body]
│      Any of init/cond/update may be NULL when omitted.
│      Restriction: initializer is an expression only, not a declaration.
│   ├─► parse_expression  (init, when non-";" token)
│   ├─► parse_expression  (condition, when non-";" token)
│   ├─► parse_expression  (update, when non-")" token)
│   └─► parse_statement   (body, executed with loop_depth++)
│
├─► "switch"  →  parse_switch_statement
│      "switch" "(" <expression> ")" <statement>
│      AST_SWITCH_STATEMENT, children: [expr, body]
│   ├─► parse_expression  (discriminant)
│   └─► parse_statement   (body, executed with switch_depth++)
│
├─► "case"  →  case label (only when switch_depth > 0)
│      "case" <integer_literal> ":" <statement>
│      AST_CASE_SECTION, children: [AST_INT_LITERAL, statement]
│   └─► parse_statement  (the statement following the label)
│
├─► "default"  →  default label (only when switch_depth > 0)
│      "default" ":" <statement>
│      AST_DEFAULT_SECTION, children: [statement]
│   └─► parse_statement  (the statement following the label)
│
├─► "{"  →  parse_block   (nested compound statement)
│      "{" { <statement> } "}"
│      AST_BLOCK, children: [statement*]
│   └─► parse_statement  (×N)
│
├─► "break"  →  AST_BREAK_STATEMENT  (requires loop_depth > 0 or switch_depth > 0)
│      "break" ";"
│
├─► "continue"  →  AST_CONTINUE_STATEMENT  (requires loop_depth > 0)
│      "continue" ";"
│
├─► "goto"  →  AST_GOTO_STATEMENT  (target label name in node->value)
│      "goto" <identifier> ";"
│
└─► expression_statement  (default: any expression used as a statement)
       <expression> ";"
       AST_EXPRESSION_STMT, children: [expression]
    └─► parse_expression  [see Section 3]
```

---

## Section 3 — Expressions

Precedence levels from lowest (outermost) to highest (innermost).
Each level delegates to the level immediately below it for its operands.

```
parse_expression
│  comma operator:  a , b , c  →  left-associative sequence
│  iterative loop over parse_assignment_expression
│  AST_COMMA_EXPR (","), each step wraps previous result as left child
└─► parse_assignment_expression
       Simple identifier assignment (lookahead fast-path):
         <identifier> <op> <assignment_expr>   right-recursive
         operators:  = += -= *= /= %= <<= >>= &= ^= |=
         compound ops desugared: a op= b  →  a = a op b (AST_BINARY_OP child)
         AST_ASSIGNMENT  (identifier stored as node->value for simple path)
       General lvalue assignment (fallthrough from conditional):
         <lvalue_expr> "=" <assignment_expr>   right-recursive
         lvalue: AST_VAR_REF, AST_DEREF, AST_ARRAY_INDEX,
                 AST_MEMBER_ACCESS, AST_ARROW_ACCESS
         AST_ASSIGNMENT  (node->value empty; lhs is children[0])
       | no assignment operator → falls through to conditional
    └─► parse_conditional
           ternary:  <logical_or> ? <expression> : <conditional>
           right-recursive; the true-branch re-enters parse_expression (top)
           AST_CONDITIONAL_EXPR, children: [condition, true_expr, false_expr]
           | no "?" → falls through to logical_or
        └─► parse_logical_or
               ||
               iterative loop; left-associative
               AST_BINARY_OP ("||")
            └─► parse_logical_and
                   &&
                   iterative loop; left-associative
                   AST_BINARY_OP ("&&")
                └─► parse_bitwise_or
                       |
                       iterative loop; left-associative
                       AST_BINARY_OP ("|")
                    └─► parse_bitwise_xor
                           ^
                           iterative loop; left-associative
                           AST_BINARY_OP ("^")
                        └─► parse_bitwise_and
                               &
                               iterative loop; left-associative
                               AST_BINARY_OP ("&")
                            └─► parse_equality
                                   ==  !=
                                   iterative loop; left-associative
                                   AST_BINARY_OP ("==" | "!=")
                                └─► parse_relational
                                       <  <=  >  >=
                                       iterative loop; left-associative
                                       AST_BINARY_OP ("<" | "<=" | ">" | ">=")
                                    └─► parse_shift
                                           <<  >>
                                           iterative loop; left-associative
                                           AST_BINARY_OP ("<<" | ">>")
                                        └─► parse_additive
                                               +  -
                                               iterative loop; left-associative
                                               AST_BINARY_OP ("+" | "-")
                                            └─► parse_term
                                                   *  /  %
                                                   iterative loop; left-associative
                                                   AST_BINARY_OP ("*" | "/" | "%")
                                                └─► parse_cast
                                                       "(" <type_name> ")" <cast_expr>
                                                       right-recursive; AST_CAST
                                                       cast to pointer: full_type set
                                                       type_name accepts: void, int types,
                                                         unsigned types, typedef names,
                                                         struct types
                                                       | no cast → falls through to unary
                                                    └─► parse_unary
                                                           prefix operators (right-recursive):
                                                             +  -  !  ~  ++  --  *  &
                                                             AST_UNARY_OP (operator as value)
                                                             AST_PREFIX_INC_DEC  (requires AST_VAR_REF)
                                                             AST_ADDR_OF (requires AST_VAR_REF or AST_ARRAY_INDEX)
                                                             AST_DEREF   (unary *)
                                                           sizeof forms:
                                                             "sizeof" <unary_expr>
                                                             "sizeof" "(" <type_name> ")"
                                                               type_name: int types, unsigned types,
                                                               "struct" tag, typedef names
                                                               sizeof(void) → compile-time error
                                                             AST_SIZEOF_EXPR | AST_SIZEOF_TYPE
                                                           | no unary op → falls through to postfix
                                                        └─► parse_postfix
                                                               iterative loop; left-associative
                                                               expr[i]   → AST_ARRAY_INDEX  children: [base, index]
                                                                 base must be AST_VAR_REF, AST_DEREF,
                                                                 or AST_ARRAY_INDEX (multi-level subscript)
                                                               expr.field → AST_MEMBER_ACCESS  (field name as value)
                                                                 children: [base_expr]
                                                               expr->field → AST_ARROW_ACCESS  (field name as value)
                                                                 children: [base_expr]
                                                               expr(args) → AST_INDIRECT_CALL  (postfix call on any expr)
                                                                 children: [callee_expr, arg*]
                                                               expr++    → AST_POSTFIX_INC_DEC ("++" as value)
                                                                 children: [base_expr]  (requires AST_VAR_REF)
                                                               expr--    → AST_POSTFIX_INC_DEC ("--" as value)
                                                                 children: [base_expr]  (requires AST_VAR_REF)
                                                            └─► parse_primary
                                                                   integer literal
                                                                     AST_INT_LITERAL
                                                                     [0-9]+ [Ll]?  →  typed by suffix
                                                                     [0-9]+ [Uu]?  →  is_unsigned set
                                                                   character literal
                                                                     AST_CHAR_LITERAL  type int
                                                                     '\x' or escape sequence
                                                                   string literal
                                                                     AST_STRING_LITERAL  type char[N+1]
                                                                     decoded payload; NUL-terminated
                                                                   enum constant
                                                                     folded to AST_INT_LITERAL (TYPE_INT)
                                                                     resolved before identifier/call dispatch
                                                                   direct function call
                                                                     AST_FUNCTION_CALL  (callee name as value)
                                                                     only when name is in the parser's function table
                                                                     arguments: parse_assignment_expression
                                                                                { "," parse_assignment_expression }
                                                                     arity validation:
                                                                       variadic: child_count >= param_count
                                                                       non-variadic: child_count == param_count
                                                                     max 6 arguments enforced at parse time
                                                                   indirect call from identifier
                                                                     AST_INDIRECT_CALL  (callee is AST_VAR_REF child)
                                                                     when name is not in the function table
                                                                     arguments: parse_assignment_expression
                                                                                { "," parse_assignment_expression }
                                                                   identifier (variable reference)
                                                                     AST_VAR_REF  (name as value)
                                                                   parenthesized expression
                                                                     "(" <expression> ")"
                                                                     re-enters parse_expression (top)

parse_initializer                          (used as initializer in declarations)
   scalar initializer: parse_assignment_expression
   brace-initializer: "{" { <initializer> [ "," ] } "}"
   AST_INITIALIZER_LIST, children: [initializer*]
   trailing comma allowed; empty "{}" produces zero-child list (zero-fill)
   nested braces allowed (recursive)
└─► parse_assignment_expression  (each element)
```

---

## Section 4 — Types and Declarators

```
parse_type_specifier
   "void"    →  TYPE_VOID
   "char"    →  TYPE_CHAR
   "short"   →  TYPE_SHORT
   "int"     →  TYPE_INT
   "long"    →  TYPE_LONG
   "unsigned" [ "char" | "short" | "int" | "long" ]
              →  unsigned variant of the named type (default: unsigned int)
              →  advances past the optional type keyword
   <typedef-name> (TOKEN_IDENTIFIER resolved via typedef table)
              →  the kind and full_type recorded at typedef registration time
              →  pointer typedefs return their full Type* chain directly
              →  unsigned scalar typedefs return their unsigned base Type*
   "enum"  →  parse_enum_specifier  (always returns type_int())
   "struct" → parse_struct_specifier  (returns TYPE_STRUCT Type*)
   advances one token for scalar/typedef cases; sub-parsers advance themselves

parse_enum_specifier
   "enum" [<tag>] "{" <enumerator_list> "}"
     enumerator_list: <identifier> [ "=" <int_literal | char_literal> ] { "," ... } [","]
     registers constants in parser->enum_consts; next_val auto-increments
     tags registered in parser->enum_tags with is_defined=1
     returns type_int()
   "enum" <tag>  (reference to a previously defined tag — tag must be defined)
     returns type_int()

parse_struct_specifier
   "struct" <tag> "{" <struct_declaration_list> "}"
     struct_declaration: <type_specifier> <declarator> { "," <declarator> } ";"
     fields use natural alignment:
       each field aligned to its type's natural alignment
       struct size rounded up to the struct's alignment (max field alignment)
       empty struct: 1 byte by convention
     field types: scalar, pointer, nested struct, array (inline field arrays)
     incomplete non-pointer field → compile-time error
     if a forward-declaration placeholder exists, completes it in-place
     TYPE_STRUCT Type* with fields[], field_count, size, alignment
   "struct" <tag>  (reference without body / forward declaration)
     creates an incomplete placeholder (size=0, alignment=0) if not yet seen
     pointer-to-incomplete is valid (allows opaque pointer fields)
   └─► parse_type_specifier  (field base types)
   └─► parse_declarator      (field declarators)

parse_type_name                            (used in cast and sizeof(type) contexts)
   [ "const" ] <type_specifier> { "*" }
   accepts: void, scalar keywords, unsigned types, typedef names, "struct" tags
   leading const qualifier silently consumed (no semantic effect on cast/sizeof)
└─► parse_type_specifier

parse_declarator                           (used in declarations and parameters)
   Plain declarator:
     { "*" } <identifier> [ "[" [ <integer_literal> ] "]" ]
     { "*" } <identifier> "("                  → is_function=1 ("(" not consumed)
   Parenthesized declarator forms:
     [outer-stars] "(" [inner-stars] <identifier> ")" "(" param-list ")"
       inner_stars > 0  → is_func_pointer=1 (function-pointer declarator)
         trailing "(" param-list ")" consumed; stored in fp_param_types/fp_param_count
         fp_outer_stars: outer stars (return-type pointer depth)
         fp_inner_stars: inner stars (normally 1)
       inner_stars == 0 → is_function=1 (redundant-paren function declaration)
     [outer-stars] "(" [inner-stars] <identifier> ")" [ "[" ... "]" ]
       → plain parenthesized pointer/array declarator
       pointer_count = outer_stars + inner_stars
   returns ParsedDeclarator:
     pointer_count   — leading stars (plain form) or combined outer+inner
     name            — identifier text
     is_array        — whether a "[" suffix was present
     array_length    — explicit size (0 when omitted)
     has_size        — whether an explicit size was provided
     is_function     — trailing "(" seen but not consumed
     is_func_pointer — full (*fp)(params) declarator (params consumed)
     fp_outer_stars, fp_inner_stars, fp_param_types, fp_param_count

build_fp_type                              (helper for function-pointer declarators)
   base_type + fp_outer_stars  →  return Type*
   type_function(return_type, params)  →  function node
   wrap fp_inner_stars times in type_pointer  →  pointer-to-function Type*

parse_declaration_specifiers               (file-scope only)
   Collects optional const qualifier + one storage class + one type specifier
   in any order.
   type qualifier: "const"
   storage class: "extern" | "static" | "typedef"
   type specifier: any form accepted by parse_type_specifier
   Errors: duplicate storage class, duplicate type specifier, missing type specifier
   Returns DeclSpecResult {sc, base_kind, base_type, is_const}
└─► parse_type_specifier

parse_parameter_declaration                (inside parameter lists)
   [ "const" ] <type_specifier> [ <declarator> | { "*" } ]
   Named parameter: AST_PARAM (name in node->value)
   Unnamed scalar: next token is "," or ")"  → AST_PARAM (empty name)
   Unnamed pointer: leading "*" stars consumed, then "," or ")"
     → AST_PARAM (empty name, decl_type=TYPE_POINTER, full_type=wrapped)
   function-pointer parameter: decl_type=TYPE_POINTER, full_type=fp chain
   leading "const" silently consumed (no semantic effect in this stage)
└─► parse_type_specifier
└─► parse_declarator  (when declarator present)
```

---

## Key Design Points

- Each expression level calls the level immediately below it for its operands.
- Recursion direction encodes associativity: right-recursive = right-associative,
  iterative loop = left-associative.
- `parse_expression` and `parse_assignment_expression` are the only expression
  levels that are right-recursive (comma is modeled as a loop but evaluates
  left-to-right; assignment is genuinely right-recursive).
- Function-call arguments use `parse_assignment_expression` directly, bypassing
  the comma level, so `f(a, b)` stays two arguments rather than one comma-expression.
- Parenthesized expressions in `parse_primary` re-enter at `parse_expression`
  (the top), making `(a, b)` a valid comma expression inside a call argument list.
- `parse_conditional`'s true-branch also re-enters at `parse_expression` (the top)
  so `a ? b, c : d` is well-formed.
- The labeled-statement lookahead in `parse_statement` peeks one extra token
  (IDENTIFIER + COLON); on mismatch the lexer position is restored before
  falling through to the expression-statement path.
- Array declarators are forbidden in multi-init-declarator lists; the parser
  rejects them with a compile-time error.
- `case` and `default` labels are parsed as statement variants and are only
  legal when `switch_depth > 0`; similarly `break`/`continue` are validated
  against `switch_depth`/`loop_depth` at parse time.
- Enum constants in `parse_primary` are folded to `AST_INT_LITERAL` before any
  identifier or function-call dispatch, so they can appear anywhere an integer
  expression is valid.
- An identifier in `parse_primary` followed by `(` that is not in the function
  table is treated as an indirect call through a function-pointer variable
  (`AST_INDIRECT_CALL`); semantic validation is deferred to codegen.
- `parse_postfix` also emits `AST_INDIRECT_CALL` for a call suffix `expr(args)`
  on an arbitrary postfix expression (e.g., `(*fp)(args)`).
- `parse_assignment_expression` has two lvalue paths: a fast-path for a plain
  identifier (saves/restores lexer state on mismatch) and a general-lvalue path
  that accepts any expression reaching `AST_DEREF`, `AST_ARRAY_INDEX`,
  `AST_MEMBER_ACCESS`, or `AST_ARROW_ACCESS` from the conditional level.
- `parse_struct_specifier` completes a forward-declaration placeholder in-place
  when a body is later provided, so any `Type*` references (e.g., in typedef
  entries) automatically reflect the completed layout without patching.
- Struct fields with an incomplete struct type (size == 0) that are not behind a
  pointer are rejected at parse time; pointer-to-incomplete is allowed for
  opaque/forward-declared types.
- `parse_initializer` is the entry point for all variable initializers; it
  unifies scalar (`parse_assignment_expression`), brace-list
  (`AST_INITIALIZER_LIST`), and nested brace forms. Only struct and array
  declarations accept brace-list initializers; scalars reject them.
- Typedef names are tracked in a scoped table (`scope_depth`); leaving a block
  removes all typedefs registered at or below that depth. The typedef table is
  consulted in `parse_type_specifier`, `parse_cast`, and `parse_statement`'s
  declaration dispatch to distinguish typedef names from ordinary identifiers.
- `parse_declaration_specifiers` at file scope handles `extern`, `static`, and
  `typedef` storage classes plus the optional `const` qualifier in combination
  with any type specifier; duplicate storage class or duplicate type specifier
  within a single declaration is an error.
- `const` qualifies the variable itself when there is no pointer depth; it is
  silently accepted but not checked on pointer targets, parameters, or in
  cast/sizeof type names.
- `unsigned` is handled inside `parse_type_specifier` and produces unsigned
  variants of char/short/int/long; bare `unsigned` defaults to unsigned int.
  The `is_unsigned` flag is propagated to AST_DECLARATION and expression nodes.
- `void` is a valid return-type specifier and a valid cast/sizeof target type
  but cannot be used as an object type (error if declared without pointer depth).
- Multi-level array subscript `a[i][j]` is supported: `parse_postfix` accepts
  `AST_ARRAY_INDEX` as a subscript base in addition to `AST_VAR_REF` and
  `AST_DEREF`.
- Variadic functions are declared with `...` as the last element of the parameter
  list. `parse_parameter_list` sets `func->is_variadic = 1` and stops consuming
  parameters. Variadic call sites require at least the fixed-parameter count;
  non-variadic sites require an exact match. The 6-argument cap applies to both.
- Abstract pointer parameters in function prototypes (e.g., `char *` with no
  name) are handled by pre-consuming leading stars in `parse_parameter_declaration`
  before checking for an absent declarator; the resulting `AST_PARAM` carries
  `decl_type=TYPE_POINTER` and an empty name.
- Array size may be omitted when an initializer is present: a brace-list
  initializer infers the size from its element count; a string-literal initializer
  infers it from the decoded byte length plus one. An explicit size that is
  smaller than a string initializer is a compile-time error; an explicit size
  with more initializers than elements is also an error (file scope only).
- Bare `return;` (no expression) is valid; `parse_statement` emits
  `AST_RETURN_STATEMENT` with no children when the token after `return` is `;`.
