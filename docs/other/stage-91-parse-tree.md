
# Full Grammar Parse Tree — Stage 91

Complete hierarchy of parser functions, grouped into four sections. Each level
calls the level(s) below it for sub-constructs. Indentation tracks call depth;
`└─►` indicates the primary descent path, `├─►` indicates a branch.

Diagnostics are raised through the `PARSER_ERROR(parser, …)` macro, which calls
`compile_error_at` with the current token's `file:line:col` position (stage
70-02/70-03). Under `--max-errors=N` the parser recovers via setjmp/longjmp and
`parser_sync()` to the next declaration boundary instead of exiting on the first
error (stage 70-01).

Since stage-75-06 the parser has closed most of the self-compilation gaps:
`for`-init declarations (76), constant-expression `case` labels (77), general
postfix chaining (78), general-lvalue compound assignment (79) and `++`/`--`
(80), `const`/`volatile` qualifiers in member and type-name contexts (82),
multidimensional arrays (86), adjacent string concatenation (89), and address-of
on member lvalues (91). As of this stage the full `src/` tree self-compiles
(see `docs/self-compilation-report.md`).

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
       extern object declarations: is_extern recorded on the global so codegen
         emits a NASM `extern` directive and skips .bss/.data emission
         (stage 84 — enables `extern FILE *stdin/stdout/stderr`).
       Variadic functions: is_variadic flag set on AST_FUNCTION_DECL.
         Stage 75-01: variadic *definitions* may have unnamed fixed
         parameters (e.g. `int f(const char *, ...)`); non-variadic
         definitions still require every parameter to be named.
       Stage 75-03: while parsing a definition body, current_func_is_variadic
         is set so __builtin_va_start can enforce its context rule.
       void object declaration → compile-time error
    ├─► parse_declaration_specifiers
    │      Collects optional const/volatile qualifiers + optional storage
    │      class + one type specifier, in any order.
    │      type qualifier: "const" | "volatile"
    │      storage class: "extern" | "static" | "typedef"
    │      type specifier: "void" | "_Bool" | scalar | "signed" | "unsigned" |
    │                      "enum" | "struct" | "union" | typedef-name
    │      Returns DeclSpecResult {sc, base_kind, base_type, is_const, is_volatile}
    │   └─► parse_type_specifier
    │
    ├─► parse_declarator  [see Section 4]
    │
    ├─► parse_parameter_list  (when function declarator)
    │      { "," <parameter_declaration> }
    │      "..." at end of list: sets func->is_variadic = 1; stops parsing
    │      Enforces unique names; unnamed (empty-name) params exempt
    │   └─► parse_parameter_declaration  (×N)
    │          [ "const" | "volatile" ] <type_specifier> [ <declarator> | { "*" } ]
    │          unnamed scalar: next token is "," or ")"
    │          unnamed pointer: leading stars consumed, then "," or ")" → AST_PARAM
    │          array param (named or unnamed): adjusted to pointer-to-element
    │            (C99 §6.7.5.3p7; stage 75-02 / 75-04)
    │          AST_PARAM  (name in node->value, empty for unnamed)
    │          func-pointer param: decl_type=TYPE_POINTER, full_type=fp chain
    │       └─► parse_type_specifier
    │
    │      File-scope array declarations (non-function):
    │        has explicit size → TYPE_ARRAY; initializer optional
    │        omitted first size + brace-initializer → size inferred from element count
    │        omitted first size + string literal  → size inferred from string length+1
    │        multidimensional (stage 86): each "[N]" suffix nests a TYPE_ARRAY
    │          (only the first dimension may be omitted; inner dims required)
    │        too many initializers for explicit size → compile-time error
    │        struct/union global initializer: brace-list (AST_INITIALIZER_LIST child)
    │          non-aggregate with brace init → compile-time error
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
├─► TOKEN_EXTERN (block scope) → error: storage class not allowed in block scope
│
├─► TOKEN_STATIC (block scope)  →  Stage 71: block-scope static local
│      Consumes "static", recursively parses the inner declaration, and
│      marks every resulting AST_DECLARATION (or each AST_DECLARATION in an
│      AST_DECL_LIST) with storage_class = SC_STATIC so codegen emits it to
│      .bss/.data with RIP-relative addressing.
│   └─► parse_statement  (the inner declaration)
│
├─► "typedef"  →  typedef declaration at block scope
│      AST_TYPEDEF_DECL  (name in node->value)
│      forms accepted: scalar, pointer, array, function-pointer typedefs
│      unsigned scalar typedefs: full_type carries the unsigned base Type*
│      struct/union typedefs: full_type carries the aggregate Type*
│      registers name in parser's typedef table with scope tracking
│   ├─► parse_type_specifier
│   └─► parse_declarator
│
├─► IDENTIFIER followed by ":"  →  labeled statement
│      AST_LABEL_STATEMENT  (label name in node->value); children: [statement]
│      (two-token lookahead; lexer position saved and restored on mismatch)
│   └─► parse_statement  (the labeled body)
│
├─► [ "const" | "volatile" ] ( "void" | "_Bool" | "char" | "short" | "int" |
│   "long" | "signed" | "unsigned" | "enum" | "struct" | "union" |
│   <typedef-name> )
│      →  declaration (scalar, pointer, array, struct, union, enum, function-pointer)
│      [ qualifiers ] <type_specifier> <init_declarator_list> ";"
│
│      Leading "const": sets local_is_const; applies to variable when no pointer depth.
│      Leading "volatile": tracked on the type (stage 82-04; no codegen effect yet).
│      void object declaration (no pointer, no array, no func-pointer) → error
│
│   ├─► parse_type_specifier
│   ├─► parse_declarator  (×N, one per init-declarator in the list)
│   │      returns ParsedDeclarator {name, pointer_count, pointer_is_const,
│   │                               is_array, array_dims[], dim_count, has_size,
│   │                               is_function, is_func_pointer, ...}
│   │
│   │  Standalone type-only declaration (no declarator, e.g. "struct S{};" ):
│   │      AST_TYPEDEF_DECL (empty name)
│   │
│   │  Single scalar/pointer declarator:
│   │      AST_DECLARATION  (name in node->value)
│   │      is_const=1 when const-qualified and no pointer depth
│   │      is_unsigned=1 when unsigned-typed scalar
│   │      optional initializer child: parse_initializer (allows brace-lists)
│   │      struct/union variable: decl_type=TYPE_STRUCT/TYPE_UNION, full_type=Type*
│   │
│   │  Single array declarator (one or more dimensions — stage 86):
│   │      AST_DECLARATION with decl_type=TYPE_ARRAY, full_type=type_array(...)
│   │      multidimensional: build_array_type_from_dims nests right-to-left
│   │        (int A[4][8] → array[4] of array[8] of int)
│   │      initializer options:
│   │        string-literal (char arrays only): AST_STRING_LITERAL child
│   │        brace-list: AST_INITIALIZER_LIST child
│   │        omitted first size + brace-list: size inferred from element count
│   │        omitted first size + string literal: size inferred from string length+1
│   │        omitted first size without either initializer → error
│   │        inner dimension omitted → error
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
│   │      struct/union members in list: each child gets the aggregate decl_type
│   │
│   └─► parse_initializer  (initializer, when "=" present)  [see Section 3]
│
├─► "return"  →  return statement
│      "return" ";"  → AST_RETURN_STATEMENT, no children
│      "return" <expression> ";"  → AST_RETURN_STATEMENT, children: [expression]
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
│      AST_WHILE_STATEMENT, children: [condition, body]
│   ├─► parse_expression  (condition)
│   └─► parse_statement   (body, loop_depth++)
│
├─► "do"  →  parse_do_while_statement
│      "do" <statement> "while" "(" <expression> ")" ";"
│      AST_DO_WHILE_STATEMENT, children: [body, condition]  (body-first order)
│   ├─► parse_statement   (body, loop_depth++)
│   └─► parse_expression  (condition)
│
├─► "for"  →  parse_for_statement
│      "for" "(" <for_init> [<expr>] ";" [<expr>] ")" <statement>
│      AST_FOR_STATEMENT, children[0..3]: [init|NULL, cond|NULL, update|NULL, body]
│      Stage 76: a scope is opened around the whole loop; the initializer clause
│        may now be a *declaration* — when the first token is a type specifier
│        (or typedef name / qualifier) it dispatches to parse_statement, otherwise
│        to parse_expression_statement. Declared variables are in scope for the
│        condition, update, and body; scope_start/local_count are saved and
│        restored across the loop boundary.
│   ├─► parse_statement / parse_expression_statement  (init: decl or expr)
│   ├─► parse_expression  (condition, when non-";" token)
│   ├─► parse_expression  (update, when non-")" token)
│   └─► parse_statement   (body, loop_depth++)
│
├─► "switch"  →  parse_switch_statement
│      AST_SWITCH_STATEMENT, children: [expr, body]
│   ├─► parse_expression  (discriminant)
│   └─► parse_statement   (body, switch_depth++)
│
├─► "case"  →  case label (only when switch_depth > 0)
│      "case" <case_constant_expr> ":" <statement>
│      AST_CASE_SECTION, children: [AST_INT_LITERAL, statement]
│      Stage 77: the label is a compile-time integer constant expression,
│        evaluated by eval_case_const_expr → eval_case_const_unary →
│        eval_case_const_primary. Accepts integer literals, character literals,
│        enum constants, and unary/binary `+`/`-` over those. Non-constant or
│        non-integer expressions → "case label expression is not an integer
│        constant expression". The folded value is stored as an AST_INT_LITERAL.
│   └─► parse_statement  (the statement following the label)
│
├─► "default"  →  default label (only when switch_depth > 0)
│      AST_DEFAULT_SECTION, children: [statement]
│   └─► parse_statement
│
├─► "{"  →  parse_block   (nested compound statement)
│
├─► "break"     →  AST_BREAK_STATEMENT     (requires loop_depth>0 or switch_depth>0)
├─► "continue"  →  AST_CONTINUE_STATEMENT  (requires loop_depth>0)
├─► "goto"      →  AST_GOTO_STATEMENT      (target label name in node->value)
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
│  AST_COMMA_EXPR (","), each step wraps previous result as left child
└─► parse_assignment_expression
       Simple identifier assignment (lookahead fast-path):
         <identifier> <op> <assignment_expr>   right-recursive
         operators:  = += -= *= /= %= <<= >>= &= ^= |=
         compound ops desugared: a op= b  →  a = a op b (AST_BINARY_OP child)
         AST_ASSIGNMENT  (identifier stored as node->value)
       General lvalue assignment (fallthrough from conditional):
         <lvalue_expr> <op> <assignment_expr>   right-recursive
         lvalue: AST_VAR_REF, AST_DEREF, AST_ARRAY_INDEX,
                 AST_MEMBER_ACCESS, AST_ARROW_ACCESS
         simple `=`: lhs is children[0]
         Stage 79: compound operators (+= … |=) also accepted on a general
           lvalue. The lhs is deep-copied via ast_clone() so it can serve both
           as the assignment target and as the left operand of the desugared
           AST_BINARY_OP without node aliasing.
         AST_ASSIGNMENT  (node->value empty; lhs is children[0])
       | no assignment operator → falls through to conditional
    └─► parse_conditional
           ternary:  <logical_or> ? <expression> : <conditional>
           right-recursive; true-branch re-enters parse_expression (top)
           AST_CONDITIONAL_EXPR, children: [condition, true_expr, false_expr]
        └─► parse_logical_or         ||   (loop; left-assoc)   AST_BINARY_OP
            └─► parse_logical_and    &&   (loop; left-assoc)   AST_BINARY_OP
                └─► parse_bitwise_or  |   (loop; left-assoc)   AST_BINARY_OP
                    └─► parse_bitwise_xor ^ (loop; left-assoc) AST_BINARY_OP
                        └─► parse_bitwise_and & (loop; left-assoc) AST_BINARY_OP
                            └─► parse_equality  == != (loop) AST_BINARY_OP
                                └─► parse_relational < <= > >= (loop) AST_BINARY_OP
                                    └─► parse_shift  << >> (loop) AST_BINARY_OP
                                        └─► parse_additive + - (loop) AST_BINARY_OP
                                            └─► parse_term * / % (loop) AST_BINARY_OP
                                                └─► parse_cast
                                                       "(" <type_name> ")" <cast_expr>
                                                       right-recursive; AST_CAST
                                                       type_name: optional const/volatile,
                                                         void, _Bool, int types,
                                                         signed/unsigned types, typedef
                                                         names, struct/union types
                                                       | no cast → falls through to unary
                                                    └─► parse_unary
                                                           prefix (right-recursive):
                                                             +  -  !  ~  ++  --  *  &
                                                             AST_UNARY_OP
                                                             AST_PREFIX_INC_DEC (general lvalue, stage 80)
                                                             AST_ADDR_OF (general lvalue, stage 91)
                                                             AST_DEREF (unary *)
                                                           ! accepts pointer operands (stage 81)
                                                           & accepts AST_VAR_REF, AST_ARRAY_INDEX,
                                                             AST_MEMBER_ACCESS, AST_ARROW_ACCESS (stage 91)
                                                           sizeof forms:
                                                             "sizeof" <unary_expr>
                                                             "sizeof" "(" <type_name> ")"
                                                               sizeof(void) → error
                                                               sizeof(int[4][8]) multidim ok (stage 86)
                                                             AST_SIZEOF_EXPR | AST_SIZEOF_TYPE
                                                           | no unary op → falls to postfix
                                                        └─► parse_postfix
                                                               iterative loop; left-associative.
                                                               Stage 78/80: suffixes chain on ANY
                                                               postfix base, not just identifiers;
                                                               lvalue validity is enforced in codegen.
                                                               expr[i]    → AST_ARRAY_INDEX [base, index]
                                                                 base may be AST_VAR_REF, AST_DEREF,
                                                                 AST_ARRAY_INDEX, AST_MEMBER_ACCESS,
                                                                 or AST_ARROW_ACCESS (stage 78);
                                                                 multidim subscript scales by inner
                                                                 array byte size (stage 86)
                                                               expr.field → AST_MEMBER_ACCESS (field as value)
                                                               expr->field → AST_ARROW_ACCESS (field as value)
                                                               expr(args) → AST_INDIRECT_CALL (postfix call)
                                                               expr++/--  → AST_POSTFIX_INC_DEC (general lvalue)
                                                            └─► parse_primary
                                                                   integer literal   → AST_INT_LITERAL
                                                                     decimal or 0x/0X hex (stage 90);
                                                                     suffix-typed; is_unsigned set for U/u
                                                                   character literal → AST_CHAR_LITERAL (int)
                                                                     hex (\xNN) / octal (\NNN) escapes (stage 88)
                                                                   string literal    → AST_STRING_LITERAL (char[N+1])
                                                                     adjacent literals concatenated in a loop
                                                                       (stage 89); hex/octal escapes decoded;
                                                                       total length checked against MAX_NAME_LEN
                                                                   __builtin_va_* intrinsics (stage 75-03),
                                                                     matched before enum/identifier dispatch:
                                                                     __builtin_va_start(ap, last)
                                                                       → AST_BUILTIN_VA_START [ap, last]
                                                                       errors outside a variadic function;
                                                                       requires exactly 2 arguments
                                                                     __builtin_va_end(ap)
                                                                       → AST_BUILTIN_VA_END [ap]
                                                                     __builtin_va_copy(dst, src)
                                                                       → AST_BUILTIN_VA_COPY [dst, src]
                                                                     __builtin_va_arg(ap, <type_name>)
                                                                       → AST_BUILTIN_VA_ARG [ap]
                                                                       2nd arg is a type-name (parse_type_name);
                                                                       result decl_type/full_type set from it
                                                                   enum constant
                                                                     folded to AST_INT_LITERAL (TYPE_INT)
                                                                   direct function call
                                                                     AST_FUNCTION_CALL (callee name as value)
                                                                     only when name is in the function table
                                                                     args: parse_assignment_expression list
                                                                     arity: variadic → count >= param_count;
                                                                            non-variadic → count == param_count
                                                                     (no maximum-argument cap — stage 68)
                                                                   indirect call from identifier
                                                                     AST_INDIRECT_CALL (callee AST_VAR_REF child)
                                                                     when name not in the function table
                                                                   identifier (variable reference) → AST_VAR_REF
                                                                   parenthesized expression
                                                                     "(" <expression> ")" → re-enters top

parse_initializer                          (used as initializer in declarations)
   scalar initializer: parse_assignment_expression
   brace-initializer: "{" { <initializer> [ "," ] } "}"
   AST_INITIALIZER_LIST, children: [initializer*]
   trailing comma allowed; empty "{}" → zero-child list (zero-fill); nested braces OK
   char-array struct members may be initialized from a string literal (stage 85)
└─► parse_assignment_expression  (each element)
```

---

## Section 4 — Types and Declarators

```
parse_type_specifier
   "void"    →  TYPE_VOID
   "_Bool"   →  TYPE_BOOL                                    (stage 63)
   "char"    →  TYPE_CHAR
   "short" [ "int" ]   →  TYPE_SHORT
   "int"     →  TYPE_INT
   "long" [ "int" ]    →  TYPE_LONG
   "long" "long" [ "int" ]  →  TYPE_LONG_LONG               (stage 64)
   "signed" [ "char" | "short" [ "int" ] | "int" |
              "long" [ "int" ] | "long" "long" [ "int" ] ]  (stage 61/64)
              →  signed variant; plain "signed" == int
              →  "signed"/"unsigned" together → error; "signed _Bool" → error
   "unsigned" [ "char" | "short" [ "int" ] | "int" |
                "long" [ "int" ] | "long" "long" [ "int" ] ] (stage 40/61/64)
              →  unsigned variant; plain "unsigned" == unsigned int
   <typedef-name> (TOKEN_IDENTIFIER resolved via typedef table)
              →  kind and full_type recorded at typedef registration time
              →  pointer / unsigned-scalar / long-long typedefs return their Type*
   "enum"   →  parse_enum_specifier   (always returns type_int())
   "struct" →  parse_struct_specifier (returns TYPE_STRUCT Type*)
   "union"  →  parse_union_specifier  (returns TYPE_UNION  Type*)   (stage 72)
   unknown token → error: expected integer type

parse_enum_specifier
   "enum" [<tag>] "{" <enumerator_list> "}"
     enumerator_list: <identifier> [ "=" <int_literal | char_literal> ] { "," ... } [","]
     registers constants in parser->enum_consts (next_val auto-increments);
     tags registered in parser->enum_tags with is_defined=1; returns type_int()
   "enum" <tag>   (reference to a previously defined tag) → type_int()

parse_struct_specifier
   "struct" [<tag>] "{" <struct_declaration_list> "}"
     struct_declaration:
       [ "const" | "volatile" ] <type_specifier> <declarator> { "," <declarator> } ";"
     natural-alignment field layout; struct size rounded up to max field alignment;
       empty struct: 1 byte by convention
     field types: scalar, pointer, nested struct/union, inline array
       (multidimensional array fields wrapped via type_array; stages 78/86)
     const member (stage 82-01): pointer fields use type_const_copy; scalar fields
       set StructField.is_const; pointer_is_const tracked for `const T *` members.
       volatile members tracked likewise (stage 82-04).
     incomplete non-pointer field → error
     forward-declaration placeholder is completed in-place when a body appears
     Stage 73-01: a body with NO tag → fresh unique anonymous TYPE_STRUCT Type*
       (type identity by pointer; structurally identical anonymous types differ)
     no tag and no body → error
     TYPE_STRUCT Type* with fields[], field_count, size, alignment
   "struct" <tag>   (reference without body) → incomplete placeholder (size=0)
       pointer-to-incomplete allowed (opaque pointer fields)
   └─► parse_type_specifier  (field base types)
   └─► parse_declarator      (field declarators)

parse_union_specifier                                         (stage 72)
   "union" [<tag>] "{" <member_list> "}"
     all members at offset 0; union size = max member size, rounded to max align
     member types: scalar, pointer, nested struct/union (incomplete non-pointer → error)
     const/volatile members tracked as for structs (stage 82-01/04)
     Stage 73-01: a body with NO tag → fresh unique anonymous TYPE_UNION Type*
     forward-declaration placeholder completed in-place when a body appears
     TYPE_UNION Type* with fields[] (offset 0), field_count, size, alignment
   "union" <tag>   (reference without body) → incomplete placeholder (size=0)
   └─► parse_type_specifier  (member base types)
   └─► parse_declarator      (member declarators)

parse_type_name                            (cast, sizeof(type), __builtin_va_arg type)
   [ "const" | "volatile" ] <type_specifier> <abstract_declarator>
   abstract_declarator: { "*" [ "const" | "volatile" ] } [ "[" [N] "]" { "[" N "]" } ]
     multidimensional abstract array declarators supported (stage 86)
   accepts: void, _Bool, scalar keywords, signed/unsigned types, typedef names,
            "struct"/"union" tags, "enum" tags
   leading const/volatile applied to the base type via type_const_copy /
     type_volatile_copy (stage 82-03/04)
└─► parse_type_specifier

parse_declarator                           (declarations and parameters)
   Leading stars with optional per-pointer const/volatile (stage 66/82-04):
     { "*" [ "const" | "volatile" ] }
     the last "const" after a star sets ParsedDeclarator.pointer_is_const
   Plain declarator:
     { "*" } <identifier> { "[" [ <integer_literal> ] "]" }   (multidim — stage 86)
       dims collected into array_dims[]/dim_count (MAX_ARRAY_DIMS=8);
       only the first dimension may omit its size
     { "*" } <identifier> "("                  → is_function=1 ("(" not consumed)
   Parenthesized declarator forms:
     [outer-stars] "(" [inner-stars] <identifier> ")" "(" param-list ")"
       inner_stars > 0  → is_func_pointer=1 (params consumed → fp_param_types/count)
       inner_stars == 0 → is_function=1 (redundant-paren function declaration)
     [outer-stars] "(" [inner-stars] <identifier> ")" [ "[" ... "]" ]
       → plain parenthesized pointer/array declarator
       pointer_count = outer_stars + inner_stars
     (*fp())(…) functions returning function pointers → error (unsupported)
     (*p)[10]  pointer-to-array → error (unsupported)
   returns ParsedDeclarator:
     pointer_count, pointer_is_const, name, array_dims[], dim_count, has_size,
     is_function, is_func_pointer, fp_outer_stars, fp_inner_stars,
     fp_param_types, fp_param_count

build_array_type_from_dims                 (helper for multidimensional arrays — stage 86)
   nests array_dims[] right-to-left into TYPE_ARRAY chains:
     int A[4][8]  →  type_array(type_array(int, 8), 4)

build_fp_type                              (helper for function-pointer declarators)
   base_type + fp_outer_stars  →  return type
   type_function(return_type, params)  →  function type
   wrap fp_inner_stars times in type_pointer  →  pointer-to-function Type*

parse_declaration_specifiers               (file-scope)
   Collects optional const/volatile + one storage class + one type specifier, any order.
   storage class: "extern" | "static" | "typedef"
   Errors: duplicate storage class, duplicate type specifier, missing type specifier
   Returns DeclSpecResult {sc, base_kind, base_type, is_const, is_volatile}
└─► parse_type_specifier

parse_parameter_declaration                (inside parameter lists)
   [ "const" | "volatile" ] <type_specifier> [ <declarator> | { "*" } ]
   Named parameter: AST_PARAM (name in node->value)
   Unnamed scalar: next token is "," or ")"  → AST_PARAM (empty name)
   Unnamed pointer: leading "*" stars consumed, then "," or ")"
     → AST_PARAM (empty name, decl_type=TYPE_POINTER)
   Array parameter (named or unnamed): adjusted to pointer-to-element
     (C99 §6.7.5.3p7; stages 75-02 / 75-04 — covers array-typedef params too)
   function-pointer parameter: decl_type=TYPE_POINTER, full_type=fp chain
   leading const/volatile silently consumed (qualifier tracked on type)
└─► parse_type_specifier
└─► parse_declarator  (when declarator present)
```

---

## Key Design Points

- Each expression level calls the level immediately below it for its operands.
  Recursion direction encodes associativity: right-recursive = right-associative,
  iterative loop = left-associative.
- `parse_expression` (comma) and `parse_assignment_expression` are the only
  right-leaning expression levels; function-call arguments use
  `parse_assignment_expression` directly, so `f(a, b)` stays two arguments.
- Parenthesized expressions and the ternary true-branch both re-enter at
  `parse_expression`, so `(a, b)` and `a ? b, c : d` are well-formed.
- **Diagnostics & recovery (stage 70):** all parser errors go through
  `PARSER_ERROR`, which attaches the current token's `file:line:col` and calls
  `compile_error_at`. With `--max-errors=N > 1` the parser uses setjmp/longjmp
  plus `parser_sync()` to resume at the next declaration boundary; the default
  (`N == 1`) exits on the first error.
- **`for`-init declarations (stage 76):** `parse_for_statement` opens a scope
  around the whole loop and lets the initializer clause be either a declaration
  or an expression; declared variables are visible in the condition, update, and
  body, and the offset bookkeeping is restored at loop exit.
- **Constant-expression `case` labels (stage 77):** a dedicated constant
  evaluator (`eval_case_const_expr`/`_unary`/`_primary`) folds integer/character
  literals, enum constants, and unary/binary `+`/`-` into a single value, so
  `case ENUM_CONST:` and `case 'A' + 1:` are now accepted.
- **General postfix chaining (stage 78):** subscript bases now include
  `AST_MEMBER_ACCESS` and `AST_ARROW_ACCESS`, and struct/union array members are
  wrapped in `TYPE_ARRAY`, enabling chains like `p->tokens[i].kind`.
- **General-lvalue assignment & `++`/`--` (stages 79/80):** compound assignment
  and prefix/postfix `++`/`--` apply to any modifiable lvalue (member access,
  subscript, deref, and chains). `ast_clone()` avoids node aliasing in the
  compound desugaring; `codegen_inc_dec_general()` handles the non-identifier
  inc/dec forms. Lvalue and const-qualification validity is enforced in codegen.
- **`!` on pointers (stage 81):** logical-not accepts pointer operands (treated
  as scalars per C99), so `if (!p)` compiles.
- **`const`/`volatile` qualifiers (stage 82):** consumed in struct/union members,
  abstract declarators, type-names (`sizeof`, casts, `va_arg`), and all
  declaration sites. `const` member writes (direct, via subscript, and through a
  pointer to a const-qualified object) are diagnosed; `volatile` is tracked with
  no codegen effect yet.
- **Function-call arity (stage 68):** the old 6-argument cap is gone. A direct
  call is validated only against the callee's signature (exact count for
  non-variadic, `>=` fixed count for variadic). 7+ arguments are handled by
  codegen via SysV AMD64 stack-passing.
- **Block-scope `static` (stage 71):** `parse_statement` accepts a leading
  `static`, parses the inner declaration, and tags the resulting
  `AST_DECLARATION` / `AST_DECL_LIST` children with `SC_STATIC`. `extern`
  remains illegal at block scope; `extern` *objects* at file scope set `is_extern`
  for `extern` directive emission (stage 84).
- **Unions (stage 72):** `parse_union_specifier` mirrors the struct path but
  places every member at offset 0 and sizes the union to its largest member.
- **Anonymous aggregates (stage 73-01):** a `struct`/`union` body with no tag
  allocates a fresh unique `Type*`; type identity is by pointer.
- **Variadic definitions & `<stdarg.h>` (stage 75):** `...` in a parameter list
  sets `is_variadic`; variadic *definitions* may leave fixed parameters unnamed.
  The four `__builtin_va_*` intrinsics are recognized in `parse_primary` before
  enum/identifier resolution; `__builtin_va_arg`'s second operand is a type-name.
- **Array-parameter decay (stages 75/86):** `parse_parameter_declaration`
  adjusts array (and array-typedef) parameter types to pointer-to-element per
  C99 §6.7.5.3p7; struct/union array members decay to pointers in value contexts
  (stage 85).
- **Multidimensional arrays (stage 86):** declarators and abstract declarators
  collect up to `MAX_ARRAY_DIMS` bracket suffixes, nested right-to-left by
  `build_array_type_from_dims`; only the first dimension may omit its size.
- **Hex/octal lexing (stages 88/90):** the lexer decodes `\xNN`/`\NNN` escapes in
  character and string literals and parses `0x`/`0X` hexadecimal integer literals
  (sharing suffix/type logic with the decimal path).
- **Adjacent string concatenation (stage 89):** `parse_primary` loops over
  consecutive `TOKEN_STRING_LITERAL` tokens, appending decoded bytes into one
  node and re-checking the `MAX_NAME_LEN` bound before each concatenation.
- **Address-of member lvalues (stage 91):** unary `&` accepts `AST_MEMBER_ACCESS`
  and `AST_ARROW_ACCESS` operands; codegen reuses `emit_member_addr` /
  `emit_arrow_addr` and sets the result type to pointer-to-field.
- Typedef names are tracked in a scoped table (`scope_depth`); leaving a block
  removes typedefs registered at or below that depth.
- `parse_struct_specifier`/`parse_union_specifier` complete a forward-declaration
  placeholder in-place when a body is later provided.
- **Self-compilation:** as of stage 91 the full `src/` tree compiles cleanly
  under the project's own compiler — the parser gaps catalogued in the
  stage-75-06 parse tree (for-init, enum case labels, member subscript/postfix,
  general-lvalue compound assignment, address-of members) are all closed. See
  `docs/self-compilation-report.md`.
- **Still parser-rejected (known gaps):** functions returning function pointers;
  pointer-to-array declarators (`(*p)[10]`); old-style (K&R) function
  definitions; designated initializers.
