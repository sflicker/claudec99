
# Full Grammar Parse Tree — Stage 21-03

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
       (currently: function declarations and definitions only)
    └─► parse_function_decl
           <type_specifier> { "*" } <identifier> "(" [ <parameter_list> ] ")" ( <block> | ";" )
           AST_FUNCTION_DECL, children: [AST_PARAM* , AST_BLOCK?]
           A pure forward-declaration has only AST_PARAM children (no body).
           A definition appends the AST_BLOCK body after the params.
        ├─► parse_type_specifier
        │      "char" | "short" | "int" | "long"
        │      advances one token; returns base Type*
        ├─► parse_parameter_list
        │      { "," <parameter_declaration> }
        │   └─► parse_parameter_declaration  (×N)
        │          <type_specifier> { "*" } <identifier>
        │          AST_PARAM  (name in node->value, type on node->decl_type / node->full_type)
        │       └─► parse_type_specifier
        └─► parse_block
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
├─► IDENTIFIER followed by ":"  →  labeled statement
│      AST_LABEL_STATEMENT  (label name in node->value)
│      children: [statement]
│      (lookahead is two tokens; lexer position is saved and restored on mismatch)
│   └─► parse_statement  (the labeled body)
│
├─► "char" | "short" | "int" | "long"  →  declaration
│      <type_specifier> <init_declarator_list> ";"
│   ├─► parse_type_specifier
│   ├─► parse_declarator  (×N, one per init-declarator in the list)
│   │      { "*" } <identifier> [ "[" [ <integer_literal> ] "]" ]
│   │      returns ParsedDeclarator {name, pointer_count, is_array, array_length, has_size}
│   │
│   │  Single declarator (scalar or pointer), no comma follows:
│   │      AST_DECLARATION  (name in node->value)
│   │      optional initializer child: parse_assignment_expression
│   │
│   │  Single declarator, array suffix present:
│   │      AST_DECLARATION with decl_type=TYPE_ARRAY, full_type=type_array(...)
│   │      optional string-literal initializer child: AST_STRING_LITERAL
│   │      (array declarators are not permitted in multi-declarator lists)
│   │
│   │  Multiple declarators (comma-separated, no array declarators):
│   │      AST_DECL_LIST, children: [AST_DECLARATION+]
│   │      each child may carry an optional initializer (parse_assignment_expression)
│   │
│   └─► parse_assignment_expression  (initializer, when "=" present)  [see Section 3]
│
├─► "return"  →  return statement
│      AST_RETURN_STATEMENT, children: [expression]
│   └─► parse_expression  [see Section 3]
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
│      "case" <integer_literal | character_literal> ":" <statement>
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
│  AST_BINARY_OP (","), each step wraps previous result as left child
└─► parse_assignment_expression
       assignment:  <unary_expr> <op> <assignment_expr>   right-recursive
       operators:  = += -= *= /= %= <<= >>= &= ^= |=
       AST_ASSIGNMENT  (operator stored as node->value)
       | no assignment operator → falls through to conditional
    └─► parse_conditional
           ternary:  <logical_or> ? <expression> : <conditional>
           right-recursive; the true-branch re-enters parse_expression (top)
           AST_CONDITIONAL, children: [condition, true_expr, false_expr]
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
                                                       | no cast → falls through to unary
                                                    └─► parse_unary
                                                           prefix operators (right-recursive):
                                                             +  -  !  ~  ++  --  *  &
                                                             AST_UNARY_OP (operator as value)
                                                           sizeof forms:
                                                             "sizeof" <unary_expr>
                                                             "sizeof" "(" <type_name> ")"
                                                             AST_SIZEOF_EXPR
                                                           | no unary op → falls through to postfix
                                                        └─► parse_postfix
                                                               iterative loop; left-associative
                                                               expr[i]   → AST_ARRAY_INDEX  children: [base, index]
                                                               expr++    → AST_POSTFIX_INC_DEC ("++" as value)
                                                               expr--    → AST_POSTFIX_INC_DEC ("--" as value)
                                                               (base must be AST_VAR_REF for ++ / --)
                                                            └─► parse_primary
                                                                   integer literal
                                                                     AST_INT_LITERAL
                                                                     [0-9]+ [Ll]?  →  typed by suffix
                                                                   character literal
                                                                     AST_CHAR_LITERAL  type int
                                                                     '\x' or escape sequence
                                                                   string literal
                                                                     AST_STRING_LITERAL  type char[N+1]
                                                                     decoded payload; NUL-terminated
                                                                   function call
                                                                     AST_FUNCTION_CALL  (callee name as value)
                                                                     arguments: parse_assignment_expression
                                                                                { "," parse_assignment_expression }
                                                                     max 6 arguments
                                                                   identifier (variable reference)
                                                                     AST_VAR_REF  (name as value)
                                                                   parenthesized expression
                                                                     "(" <expression> ")"
                                                                     re-enters parse_expression (top)
```

---

## Section 4 — Types and Declarators

```
parse_type_specifier
   "char"  →  TYPE_CHAR
   "short" →  TYPE_SHORT
   "int"   →  TYPE_INT
   "long"  →  TYPE_LONG
   advances one token; returns base Type*

parse_type_name                        (used in cast and sizeof(type) contexts)
   <type_specifier> { "*" }
└─► parse_type_specifier

parse_declarator                       (used in declarations)
   { "*" } <identifier> [ "[" [ <integer_literal> ] "]" ]
   returns ParsedDeclarator:
     pointer_count  — number of leading stars
     name           — identifier text
     is_array       — whether a bracket suffix was present
     array_length   — explicit size (0 when omitted)
     has_size       — whether an explicit size was provided
└─► parse_type_specifier  (called by the enclosing declaration, not parse_declarator itself)
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
