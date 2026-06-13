
# Full Grammar Parse Tree — Stage 111

Complete hierarchy of parser functions, grouped into four sections. Each level
calls the level(s) below it for sub-constructs. Indentation tracks call depth;
`└─►` indicates the primary descent path, `├─►` indicates a branch.

Diagnostics are raised through the `PARSER_ERROR(parser, …)` macro, which calls
`compile_error_at` with the current token's `file:line:col` position (stage
70-02/70-03). Under `--max-errors=N` the parser recovers via setjmp/longjmp and
`parser_sync()` to the next declaration boundary instead of exiting on the first
error (stage 70-01).

Since stage-75-06 the parser has closed every self-compilation gap:
`for`-init declarations (76), constant-expression `case` labels (77), general
postfix chaining (78), general-lvalue compound assignment (79) and `++`/`--`
(80), `const`/`volatile` qualifiers in member and type-name contexts (82),
multidimensional arrays (86), adjacent string concatenation (89), address-of
on member lvalues (91), and struct/union by-value parameters and returns (91-01).
Stage 92 converted `ASTNode.children` from a fixed 64-slot array into a
lazily-allocated doubling dynamic array (large translation units, blocks, and
switches were silently truncated before), which finally let the full `src/`
tree bootstrap. **As of stage 92 the compiler is fully self-hosting:** C0
(gcc-built) compiles itself to C1, and C1 compiles itself to C2, with all 1479
tests passing at each step (see `docs/self-compilation-report.md`).

**Stages 91-01 through 95-12 carry no expression-grammar or AST-shape changes.**
Stage 91-01 (struct-by-value) and stage 93/94 (build workflow / self-host cycle)
are codegen/ABI/tooling work, and the entire 95-xx series is an internal
storage refactor: stages 95-02/95-03 add the reusable `Vec` and `StrBuf`
dynamic containers; stages 95-04 through 95-07 replace the parser/codegen
fixed-capacity tables with `Vec`, removing their hard caps; and stages 95-08
through 95-11 move all token, AST, parser, and codegen name/tag/label text from
`char[MAX_NAME_LEN]` buffers into `const char *` pointers backed by a
lexer-owned string pool — which removes the old 255-byte string-literal cap
entirely. Stage 95-12 closes the last two fixed-capacity items.

**Stage 96 is a driver/lifecycle stage — no expression-grammar, parser function,
or AST node changes.** The following new lifecycle functions are added:

- `parser_free(Parser *)` (`src/parser.c`)
- `codegen_free(CodeGen *)` (`src/codegen.c`)
- `reset_error_state()` (`src/util.c`)

The driver (`src/compiler.c`) now accepts one or more positional source files.

**Stage 97 adds `AST_DESIGNATED_INIT` and `parse_initializer_element`.**
Two changes: new AST node `AST_DESIGNATED_INIT` (`include/ast.h`) inside
`AST_INITIALIZER_LIST`; new helper `parse_initializer_element` detects `.`
(member designator) or `[` (array index designator) before delegating to
`parse_initializer`. Array indices use `eval_case_const_expr`.

**Stage 98 adds `AST_COMPOUND_LITERAL` and the compound literal expression
grammar.** `parse_cast` extended: after `(type-name)`, if the next token is
`TOKEN_LBRACE`, a compound literal is built by `build_compound_literal` +
`parse_postfix_suffixes`. `parse_postfix` tries compound literal detection
before `parse_primary`. `parse_unary` extended: `&` now accepts
`AST_COMPOUND_LITERAL` operands.

**Stage 99 extends the integer constant expression evaluator and enables
`typedef enum` forward references.** No new AST node types and no new
grammar productions. `eval_const_expr` replaces `eval_case_const_*`:
nine-level recursive descent with operators `* / % << >> & ^ |`, unary
`~ !`, and parenthesized sub-expressions. `parse_enum_specifier` accepts
forward-declared tags via a forward-declaration entry in `enum_tags`.

**Stage 100 extends `eval_const_primary` with `sizeof` and wires constant
expressions into file-scope integer initializers.** `eval_const_primary`
now handles `TOKEN_SIZEOF` — `sizeof(type-name)` is evaluated at parse
time to an integer constant. In `parse_external_declaration`, the first-declarator
and multi-declarator file-scope integer-typed initializer paths now call
`eval_const_expr` instead of requiring a literal, so `int X = 1 + 2;`,
`int Y = sizeof(int) * 1024;`, and `int Z = ~0;` all work at file scope.
No new AST node types; the folded value is stored as `AST_INT_LITERAL`.

**Stage 104 completes the integer constant expression evaluator with
relational, equality, logical, and ternary operators.** Five new levels
added to `src/parser.c`:

- `eval_const_relational` — `<`, `<=`, `>`, `>=`
- `eval_const_equality` — `==`, `!=`
- `eval_const_logical_and` — `&&`
- `eval_const_logical_or` — `||`
- `eval_const_conditional` — `?:` (lazy — evaluates only the selected branch)

The evaluator chain is now: `eval_const_expr → eval_const_conditional →
eval_const_logical_or → eval_const_logical_and → eval_const_equality →
eval_const_relational → eval_const_bitwise_or → …`. A pre-existing
precedence bug (swapped call order of `eval_const_additive` and
`eval_const_shift`) was fixed; `3+1<<2` now evaluates to 16 (not 7).
The same five operators were added to `eval_const_init` in `src/codegen.c`
for block-scope static scalar initializers.

**Stage 106 adds `TOKEN_RESTRICT` — parse-and-ignore at all pointer
qualifier positions.** `parse_declaration_specifiers` now consumes
`TOKEN_RESTRICT` alongside `const` and `volatile`. The leading-star loop
in `parse_declarator`, the abstract declarator star loop in `parse_type_name`,
and the qualifier-handling in `parse_parameter_declaration` all accept
`TOKEN_RESTRICT` as a qualifier that is parsed and silently discarded with
no AST or semantic effect.

**Stage 107 adds `TOKEN_INLINE` — parse-and-ignore in `parse_declaration_specifiers`.**
`inline` is consumed and discarded alongside `restrict`, `volatile`, and the
storage-class specifiers. No AST changes. Additionally `AST_BUILTIN_VA_COPY`
is split from `AST_BUILTIN_VA_END` in `parse_primary` so that codegen can
emit distinct (functional) code for `va_copy`.

**Stage 108 adds `#elifdef` and `#elifndef` — preprocessor only.** No
lexer token changes, no parser changes, no AST node changes.

**Stage 109 adds `float` and `double` as first-class scalar types.** New
tokens: `TOKEN_FLOAT`, `TOKEN_DOUBLE` (keyword tokens), `TOKEN_FLOAT_LITERAL`,
`TOKEN_DOUBLE_LITERAL` (literal tokens). New AST node: `AST_FLOAT_LITERAL`
(single type for both float and double; `decl_type` field distinguishes them).
Parser changes:
- `parse_type_specifier` handles `TOKEN_FLOAT` / `TOKEN_DOUBLE`
- `parse_primary` creates `AST_FLOAT_LITERAL` from `TOKEN_FLOAT_LITERAL` /
  `TOKEN_DOUBLE_LITERAL`
- All type-start lookahead sets updated (compound literal, sizeof, cast,
  for-init, statement dispatch, file-scope dispatch, const-expr sizeof)
- File-scope FP initializers accepted via `parse_assignment_expression` +
  `AST_FLOAT_LITERAL` validation

**Stages 102, 103, 105, 108, 110, and 111 have no parser or AST changes.**
Stages 102–103 extend codegen for block-scope static aggregates and scalar
constant-expression initializers. Stage 110 adds FP arithmetic codegen.
Stage 111 adds FP comparison and boolean-context codegen.

---

## Section 1 — Program Structure

```
parse_translation_unit
│  one or more external-declarations until EOF
│  AST_TRANSLATION_UNIT, children: [external_declaration+]
│  Stage 92: children live in a lazily-allocated doubling array (ast_add_child),
│    so a TU with hundreds of top-level decls is no longer truncated at 64.
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
       Stage 92: non-static file-scope objects defined in this TU are marked
         is_static_linkage=0 and emitted with a NASM `global` directive; an
         object both defined and self-declared `extern` no longer double-emits.
       Variadic functions: is_variadic flag set on AST_FUNCTION_DECL.
         Stage 75-01: variadic *definitions* may have unnamed fixed
         parameters (e.g. `int f(const char *, ...)`); non-variadic
         definitions still require every parameter to be named.
       Stage 75-03: while parsing a definition body, current_func_is_variadic
         is set so __builtin_va_start can enforce its context rule.
       Stage 91-01: a struct/union *return type* now carries its full_type so
         codegen can classify it (register-class ≤16 bytes vs memory-class
         returned through a hidden pointer); the type was previously NULL.
       void object declaration → compile-time error
       Stage 99: file-scope initializer uses parse_assignment_expression (not
         parse_primary) for the non-brace path; AST_COMPOUND_LITERAL in a
         file-scope initializer → "compound literals at file scope not yet supported"
       Stage 100: integer-typed file-scope initializer (non-pointer) calls
         eval_const_expr; sizeof(type-name) is now valid in that context.
    ├─► parse_declaration_specifiers
    │      Collects optional qualifiers + optional storage class + one type specifier,
    │      in any order.
    │      type qualifier: "const" | "volatile" | "restrict" (stage 106, parse-and-ignore)
    │      storage class: "extern" | "static" | "typedef"
    │      function specifier: "inline" (stage 107, parse-and-ignore)
    │      type specifier: "void" | "_Bool" | scalar | "float" | "double" (stage 109) |
    │                      "signed" | "unsigned" | "enum" | "struct" | "union" |
    │                      typedef-name
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
    │          [ "const" | "volatile" | "restrict" ] <type_specifier> [ <declarator> | { "*" } ]
    │          unnamed scalar: next token is "," or ")"
    │          unnamed pointer: leading stars consumed, then "," or ")" → AST_PARAM
    │          array param (named or unnamed): adjusted to pointer-to-element
    │            (C99 §6.7.5.3p7; stage 75-02 / 75-04)
    │          struct/union value param (stage 91-01): full_type attached so
    │            codegen can apply SysV classification at the call site and in the
    │            prologue (register-class structs bound by direct stores;
    │            memory-class structs copied from the stack into a private local)
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
    │        Stage 109: file-scope float/double initializer: AST_FLOAT_LITERAL child
    │
    └─► parse_block  (when definition)
           "{" { <statement> } "}"
           AST_BLOCK, children: [statement*]  (dynamic array — stage 92)
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
├─► [ "const" | "volatile" | "restrict" ] ( "void" | "_Bool" | "char" | "short" |
│   "int" | "long" | "signed" | "unsigned" | "float" | "double" (stage 109) |
│   "enum" | "struct" | "union" | <typedef-name> )
│      →  declaration (scalar, pointer, array, struct, union, enum, function-pointer)
│      [ qualifiers ] <type_specifier> <init_declarator_list> ";"
│
│      Leading "const": sets local_is_const; applies to variable when no pointer depth.
│      Leading "volatile": tracked on the type (stage 82-04; no codegen effect yet).
│      Leading "restrict": parse-and-ignore (stage 106).
│      void object declaration (no pointer, no array, no func-pointer) → error
│
│   ├─► parse_type_specifier
│   ├─► parse_declarator  (×N, one per init-declarator in the list)
│   │      returns ParsedDeclarator {name, pointer_count, pointer_is_const,
│   │                               is_array, array_dims[], dim_count, has_size,
│   │                               is_function, is_func_pointer, ...}
│   │
│   │  Single scalar/pointer declarator:
│   │      AST_DECLARATION  (name in node->value)
│   │      Stage 109: float/double declarations allocate 4/8 bytes with proper
│   │        alignment; initializer from AST_FLOAT_LITERAL stored in xmm0
│   │      optional initializer child: parse_initializer (allows brace-lists)
│   │      struct/union variable: decl_type=TYPE_STRUCT/TYPE_UNION, full_type=Type*
│   │
│   │  Single array declarator (one or more dimensions — stage 86):
│   │      AST_DECLARATION with decl_type=TYPE_ARRAY, full_type=type_array(...)
│   │      multidimensional: build_array_type_from_dims nests right-to-left
│   │      initializer options: string-literal, brace-list (may include
│   │        AST_DESIGNATED_INIT nodes — stage 97), omitted-size inference
│   │
│   │  Function-pointer declarator (*fp)(...):
│   │      AST_DECLARATION with decl_type=TYPE_POINTER, full_type=fp chain
│   │
│   │  Multiple declarators (comma-separated; array/func-pointer forbidden):
│   │      AST_DECL_LIST, children: [AST_DECLARATION+]
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
│      Stage 111: FP condition types handled in codegen via emit_fp_bool_to_rax;
│        no parser change.
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
│      AST_DO_WHILE_STATEMENT, children: [body, condition]
│   ├─► parse_statement   (body, loop_depth++)
│   └─► parse_expression  (condition)
│
├─► "for"  →  parse_for_statement
│      "for" "(" <for_init> [<expr>] ";" [<expr>] ")" <statement>
│      AST_FOR_STATEMENT, children[0..3]: [init|NULL, cond|NULL, update|NULL, body]
│      Stage 76: a scope is opened around the whole loop; the initializer clause
│        may now be a *declaration*.
│      Stage 92: the for-node builder appends children via ast_add_child.
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
│        evaluated by eval_const_expr(parser, "case label expression").
│        Stage 99: evaluator extended to multiplicative (* / %), shift (<< >>),
│        bitwise (& ^ |), unary (~ !), and parenthesized sub-expressions.
│        Stage 104: evaluator further extended to relational (<, <=, >, >=),
│        equality (==, !=), logical (&&, ||), and ternary (?:) operators.
│        Non-constant or non-integer expressions → error.
│        The folded value is stored as an AST_INT_LITERAL.
│        (Codegen collects the labels into a per-switch `Vec` of
│        `SwitchLabel{node,label}`; the case/default count per switch is
│        unbounded since stage 95-12.)
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
         struct/union `=` from a struct rvalue accepted; codegen block-copies
           the value into the lvalue (stage 91-01)
       General lvalue assignment (fallthrough from conditional):
         <lvalue_expr> <op> <assignment_expr>   right-recursive
         lvalue: AST_VAR_REF, AST_DEREF, AST_ARRAY_INDEX,
                 AST_MEMBER_ACCESS, AST_ARROW_ACCESS
         simple `=`: lhs is children[0]
         Stage 79: compound operators (+= … |=) also accepted on a general
           lvalue. The lhs is deep-copied via ast_clone() so it can serve both
           as the assignment target and as the left operand of the desugared
           AST_BINARY_OP without node aliasing.
         Stage 91-01: whole-struct assignment via subscript/dot/arrow/deref.
         AST_ASSIGNMENT  (node->value empty; lhs is children[0])
       | no assignment operator → falls through to conditional
    └─► parse_conditional
           ternary:  <logical_or> ? <expression> : <conditional>
           right-recursive; true-branch re-enters parse_expression (top)
           AST_CONDITIONAL_EXPR, children: [condition, true_expr, false_expr]
           Stage 111: if condition result_type is FP, codegen calls
             emit_fp_bool_to_rax before the je; no parser change.
        └─► parse_logical_or         ||   (loop; left-assoc)   AST_BINARY_OP
            └─► parse_logical_and    &&   (loop; left-assoc)   AST_BINARY_OP
                └─► parse_bitwise_or  |   (loop; left-assoc)   AST_BINARY_OP
                    └─► parse_bitwise_xor ^ (loop; left-assoc) AST_BINARY_OP
                        └─► parse_bitwise_and & (loop; left-assoc) AST_BINARY_OP
                            └─► parse_equality  == != (loop) AST_BINARY_OP
                                │  Stage 111: == and != on FP operands use
                                │    ucomisd/ucomiss + NaN-correct set* sequences
                                └─► parse_relational < <= > >= (loop) AST_BINARY_OP
                                    │  Stage 111: relational on FP operands use
                                    │    ucomisd/ucomiss + setb/setbe/seta/setae
                                    └─► parse_shift  << >> (loop) AST_BINARY_OP
                                        └─► parse_additive + - (loop) AST_BINARY_OP
                                            └─► parse_term * / % (loop) AST_BINARY_OP
                                                └─► parse_cast
                                                       Stage 98: "(type-name) {init}"
                                                         → build_compound_literal + parse_postfix_suffixes
                                                       "(type-name) <cast_expr>"
                                                         → regular cast: AST_CAST
                                                         type_name includes float/double
                                                           (stage 109)
                                                       | no cast → falls through to unary
                                                    └─► parse_unary
                                                           prefix (right-recursive):
                                                             +  -  !  ~  ++  --  *  &
                                                             AST_UNARY_OP
                                                             AST_PREFIX_INC_DEC (general lvalue, stage 80)
                                                             AST_ADDR_OF (general lvalue, stage 91)
                                                             AST_DEREF (unary *)
                                                           ! accepts pointer operands (stage 81)
                                                           Stage 111: ! on FP operands uses
                                                             ucomiss/ucomisd + sete+setnp+and
                                                           & accepts AST_VAR_REF, AST_ARRAY_INDEX,
                                                             AST_MEMBER_ACCESS, AST_ARROW_ACCESS (stage 91)
                                                             AST_COMPOUND_LITERAL (stage 98 — lvalue)
                                                           sizeof forms:
                                                             "sizeof" <unary_expr>
                                                             "sizeof" "(" <type_name> ")"
                                                               sizeof(void) → error
                                                               sizeof(float) = 4 (stage 109)
                                                               sizeof(double) = 8 (stage 109)
                                                               sizeof(int[4][8]) multidim ok (stage 86)
                                                               sizeof a string literal → strlen+1
                                                             AST_SIZEOF_EXPR | AST_SIZEOF_TYPE
                                                           | no unary op → falls to postfix
                                                        └─► parse_postfix
                                                               Stage 98: tries compound literal detection
                                                               before parse_primary.
                                                               Iterative suffix loop (left-assoc).
                                                               Stage 78/80: suffixes chain on ANY
                                                               postfix base, not just identifiers.
                                                               expr[i]    → AST_ARRAY_INDEX [base, index]
                                                               expr.field → AST_MEMBER_ACCESS
                                                               expr->field → AST_ARROW_ACCESS
                                                               expr(args) → AST_INDIRECT_CALL
                                                               expr++/--  → AST_POSTFIX_INC_DEC
                                                            └─► parse_primary
                                                                   integer literal   → AST_INT_LITERAL
                                                                     decimal or 0x/0X hex (stage 90);
                                                                     suffix-typed; is_unsigned set for U/u
                                                                   float literal     → AST_FLOAT_LITERAL
                                                                     TOKEN_FLOAT_LITERAL → decl_type=TYPE_FLOAT
                                                                     TOKEN_DOUBLE_LITERAL → decl_type=TYPE_DOUBLE
                                                                     (stage 109)
                                                                   character literal → AST_CHAR_LITERAL (int)
                                                                     hex (\xNN) / octal (\NNN) escapes (stage 88)
                                                                   string literal    → AST_STRING_LITERAL
                                                                     adjacent literals concatenated (stage 89)
                                                                   __builtin_va_* intrinsics (stage 75-03):
                                                                     __builtin_va_start → AST_BUILTIN_VA_START
                                                                     __builtin_va_end   → AST_BUILTIN_VA_END
                                                                     __builtin_va_copy  → AST_BUILTIN_VA_COPY
                                                                       (split from VA_END in stage 107;
                                                                       codegen emits three 8-byte copies)
                                                                     __builtin_va_arg   → AST_BUILTIN_VA_ARG
                                                                   enum constant
                                                                     folded to AST_INT_LITERAL (TYPE_INT)
                                                                   direct function call
                                                                     AST_FUNCTION_CALL (callee name as value)
                                                                   indirect call from identifier
                                                                     AST_INDIRECT_CALL
                                                                   identifier (variable reference) → AST_VAR_REF
                                                                     Stage 109: FP var-refs use movss/movsd loads
                                                                   parenthesized expression
                                                                     "(" <expression> ")" → re-enters top

parse_initializer                          (used as initializer in declarations)
   scalar initializer: parse_assignment_expression
     Stage 98: scalar initializer may be an AST_COMPOUND_LITERAL
     Stage 109: FP scalar initializer may be an AST_FLOAT_LITERAL
   brace-initializer: "{" { <initializer_element> [ "," ] } "}"
   AST_INITIALIZER_LIST, children: [initializer_element*]
   trailing comma allowed; empty "{}" → zero-child list (zero-fill); nested braces OK
   char-array struct members may be initialized from a string literal (stage 85)
   Stage 97: each element is dispatched through parse_initializer_element
└─► parse_initializer_element  (stage 97: each list element)
       "." <identifier> "=" <initializer>
         → AST_DESIGNATED_INIT (value = field name, byte_length = 0)
            child[0]: parse_initializer result
         chained designator detected → error "chained designators not yet supported"
       "[" <const_integer_expr> "]" "=" <initializer>
         → AST_DESIGNATED_INIT (value = NULL, byte_length = const_index)
            child[0]: parse_initializer result
         index expression evaluated by eval_const_expr (stage 99)
       anything else: parse_initializer (non-designated element)

eval_const_expr                            (parser token-stream constant evaluator)
   nine-level recursive descent (stage 99):
     eval_const_expr
     └─► eval_const_conditional (?:, lazy, stage 104)
         └─► eval_const_logical_or (||, stage 104)
             └─► eval_const_logical_and (&&, stage 104)
                 └─► eval_const_equality (== !=, stage 104)
                     └─► eval_const_relational (< <= > >=, stage 104)
                         └─► eval_const_bitwise_or (|)
                             └─► eval_const_bitwise_xor (^)
                                 └─► eval_const_bitwise_and (&)
                                     └─► eval_const_additive (+ -)
                                         └─► eval_const_shift (<< >>)
                                             └─► eval_const_multiplicative (* / %)
                                                 └─► eval_const_unary (+ - ~ !)
                                                     └─► eval_const_primary
                                                           TOKEN_INT_LITERAL (decimal/hex)
                                                           TOKEN_CHAR_LITERAL
                                                           TOKEN_SIZEOF + "(type-name)"
                                                             (stage 100; evaluates to type size)
                                                           enum constant by name
                                                           "(" eval_const_expr ")"
   Context string parameter "case label expression", "enumerator value",
     "array designator index", "file-scope initializer" identifies call site in errors.
   Division/modulo by zero → PARSER_ERROR.
   Stage 100: sizeof(type-name) supported; eval_const_primary handles TOKEN_SIZEOF.
   Stage 104: relational, equality, logical-and, logical-or, ternary added.
   Additive/shift precedence bug fixed in stage 104 (swapped call order corrected).
```

---

## Section 4 — Types and Declarators

```
parse_type_specifier
│  Matches a single type keyword or a named type:
│  "void"             → TYPE_VOID
│  "_Bool"            → TYPE_BOOL
│  "char"             → TYPE_CHAR
│  "short" ["int"]    → TYPE_SHORT
│  "int"              → TYPE_INT
│  "long" ["int"]     → TYPE_LONG (+ "long" "long" ["int"] → TYPE_LONG_LONG)
│  "signed" …         → signed variant of the above
│  "unsigned" …       → unsigned variant (TYPE_UNSIGNED_CHAR/SHORT/INT/LONG/LONG_LONG)
│  "float"            → TYPE_FLOAT   (stage 109 — 4 bytes, 4-byte alignment)
│  "double"           → TYPE_DOUBLE  (stage 109 — 8 bytes, 8-byte alignment)
│  "enum" …           → integer type (TYPE_INT); see parse_enum_specifier
│  "struct" / "union" → aggregate type; see parse_struct/union_specifier
│  typedef name       → typedef chain

parse_declarator  (stage 95-08: name stored as const char *)
│  Parses pointer stars and "restrict" (stage 106, ignore) qualifiers.
│  Leading stars: each "*" increments pointer_count in ParsedDeclarator.
│    Qualifiers after "*" (const/volatile/restrict): tracked or ignored.
│  Parenthesized form "(*name)" or "(**name)": grouping for pointer-to-pointer.
│  Array suffix "[N]": each "[N]" appends a dimension to array_dims[];
│    omitted size in first position only (for function params, file-scope,
│    and compound literal base types).
│  Function suffix "(...)": is_function or is_func_pointer set.

parse_type_name  (used in sizeof, cast, compound-literal, va_arg)
│  [ const | volatile | restrict ] <type_specifier> [ abstract_declarator ]
│  abstract_declarator: leading "*", optional qualifiers, optional "[N]"
│  Stage 109: "float" and "double" accepted as type specifiers.

parse_struct_specifier / parse_union_specifier
│  "struct"/"union" <tag> "{" { <field_declaration> } "}"  → new type
│  "struct"/"union" <tag>                                   → reference to existing
│  anonymous (no tag, body present): unique type allocated per definition
│  Stage 85: member arrays of char may be initialized from a string literal in
│    brace initializers; member-array fields decay to pointer in value contexts.
│  Stage 109: struct/union fields may have type float or double (4/8 bytes with
│    proper alignment and SSE2 movss/movsd load/store in codegen).

parse_enum_specifier
│  "enum" <tag> "{" { <enumerator> [ "," ] } "}"  → integer type (TYPE_INT)
│  "enum" <tag>  →  reference (forward ref if not in table — stage 99)
│  enumerator: <name> [ "=" eval_const_expr ]
│    stage 99: value may be any integer constant expression (not just a literal)
│  Forward-declared tags: unknown tag creates EnumTag {tag, is_defined=0};
│    body later marks is_defined=1. Enables `typedef enum Status Status;` before body.

## Key Design Points (through Stage 111)

1. **Self-hosting constraint**: the compiler cannot use C99 VLAs in its own
   source (it cannot yet compile VLA code). All local arrays in the parser/codegen
   that sized themselves from runtime values have been converted to `Vec` or fixed-
   capacity arrays checked before write.

2. **String pool ownership**: since stage 95-08, all token text lives in a
   `lexer->str_pool` `Vec<char *>`. `parser->current.value` is a `const char *`
   into the pool. AST nodes, parser structs, and codegen structs hold `const char *`
   pointers — no embedded `char[N]` buffers remain in any of these structures.

3. **Dynamic children**: `ASTNode.children` is a lazily-allocated, doubling array
   managed by `ast_add_child` (stage 92). The initial capacity is `AST_MAX_CHILDREN`
   (64) but there is no cap — translation units with hundreds of top-level decls
   and switches with hundreds of case labels are handled correctly.

4. **Floating-point representation**: FP values live in SSE2 registers (`xmm0`–
   `xmm7`). Stage 109 adds load/store helpers for FP locals and globals.
   Stage 110 adds arithmetic. Stage 111 adds comparison and boolean-context
   codegen. The parser and AST have a single node type `AST_FLOAT_LITERAL`
   for both float and double, distinguished by `decl_type`.

5. **Integer constant expression evaluator**: `eval_const_expr` in `src/parser.c`
   is now a complete 13-level evaluator supporting all C99 integer constant
   expression operators including relational, equality, logical-and/or, and
   ternary (stage 104). The same operators are mirrored in `eval_const_init`
   in `src/codegen.c` for block-scope static scalar initializers (stage 103–104).

6. **Known gaps** (as of stage 111):
   - FP function parameters and return values (stage 112).
   - `float`/`double` in `va_arg` calls (stage 112).
   - Bit-fields, flexible array members, `__attribute__`, K&R definitions.
   - Object-file (`.o`) emission; separate linking.
   - Compound literals at file scope.
   - Block-scope `extern` declarations.
```
