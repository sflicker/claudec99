# Full Grammar Parse Tree — Stage 104

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
(gcc-built) compiles itself to C1, and C1 compiles itself to C2, with all 1584
tests passing at each step (see `docs/self-compilation-report.md`).

**Stages 91-01 through 103 carry no expression-grammar or AST-shape changes.**
Stages 91-01 (struct-by-value), 93/94 (build workflow / self-host cycle), and
the entire 95-xx series are codegen/ABI/tooling work. Stages 96-98 are
language-scoped (multi-file compilation, designated initializers, compound
literals), and stages 100-103 extend the constant-expression evaluator into
three new contexts: file-scope initializers (stage 100), block-scope static
aggregate initializers (stage 101-102), and block-scope static scalar
initializers with full constant-expression support (stage 103).

**Stage 104 extends the constant-expression evaluators only** — no new AST node
types, no new grammar rules, and no changes to the main expression-parser
function chain.

---

**Stage 100 — File-Scope Constant Expressions.**

File-scope scalar globals can now be initialized with full compile-time integer
constant expressions (arithmetic, bitwise `& ^ |`, shift `<< >>`, multiplicative
`* / %`, unary `~ ! +`, and `sizeof(type-name)`), not just literals.
The parser path is unchanged; the initializer is now evaluated by `eval_const_expr`
instead of the literal-only check. No new AST nodes, no codegen changes.

**Stage 101 — Block-Scope Static Aggregates.**

Block-scope `static` declarations now support array, struct, and union types
(previously only scalars and pointers). New codegen field: `LocalStaticVar.init_node`
carries the initializer for aggregate statics. RIP-relative addressing applies to
array decay, subscript, and member access. No parser changes.

**Stage 102 — Complete Static Aggregate Coverage.**

Designated initializers (`[index]` and `.member`) now work in block-scope static
arrays and file-scope global arrays. Multidimensional static arrays are now
supported (both locally and globally). Codegen uses a slots array to track
designated positions and zero-fills the remainder. No parser changes.

**Stage 103 — Block-Scope Static Scalar Constant-Expression Initializers.**

Block-scope `static` scalar variables now accept the full constant-expression
evaluator (`eval_const_init`) instead of a 3-case literal-only check. This
enables `static int x = 1 + 2 * 3;`, `static int y = (1 << 4) | 3;`, etc.
Codegen-only change; no parser modifications. `eval_const_init` handles all
the operators that `eval_const_expr` handles, plus integer/character literals.

**Stage 104 — Complete Constant-Expression Evaluator.**

Both `eval_const_expr` (token-stream, `src/parser.c`) and `eval_const_init`
(AST-based, `src/codegen.c`) are extended to support the full C99 integer
constant expression operator set: relational (`<`, `<=`, `>`, `>=`), equality
(`==`, `!=`), logical-AND (`&&`), logical-OR (`||`), and the ternary conditional
(`?:`). Also fixes a pre-existing additive/shift precedence bug: `eval_const_additive`
now correctly calls `eval_const_multiplicative` (not `eval_const_shift`), and
`eval_const_shift` now correctly calls `eval_const_additive` (not
`eval_const_multiplicative`). No new AST node types, no new tokens, no grammar
rules for the main expression parser.

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
       Stage 100: file-scope scalar integer globals use eval_const_expr instead
         of literal-only check
       Stage 104: eval_const_expr now supports relational, equality, logical,
         and ternary operators in file-scope initializers
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
    │        Stage 100: scalar integer globals use eval_const_expr for initializers
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
│      Stage 101: block-scope static *arrays* and *structs/unions* now supported
│        (previously only scalars and pointers); initializer carried in init_node.
│      Stage 103: scalar static initializers use eval_const_init for full
│        constant-expression support (was literal-only check).
│      Stage 104: eval_const_init extended to relational, equality, logical,
│        and ternary operators.
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
│   │        Stage 99: initializer may now be an AST_COMPOUND_LITERAL (e.g.
│   │          `struct Point p = (struct Point){.x=1}`)
│   │      struct/union variable: decl_type=TYPE_STRUCT/TYPE_UNION, full_type=Type*
│   │      struct/union decl-init from a struct rvalue (variable, call result, or
│   │        copy) accepted; codegen block-copies into the new local (stage 91-01)
│   │      Stage 103: static scalar initializers use eval_const_init for full
│   │        constant-expression support
│   │      Stage 104: eval_const_init extended to relational, equality,
│   │        logical, and ternary operators
│   │
│   │  Single array declarator (one or more dimensions — stage 86):
│   │      AST_DECLARATION with decl_type=TYPE_ARRAY, full_type=type_array(...)
│   │      multidimensional: build_array_type_from_dims nests right-to-left
│   │        (int A[4][8] → array[4] of array[8] of int)
│   │      initializer options:
│   │        string-literal (char arrays only): AST_STRING_LITERAL child
│   │        brace-list: AST_INITIALIZER_LIST child
│   │          elements may be AST_DESIGNATED_INIT nodes (stage 97)
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
│      Stage 91-01: when the function returns a struct/union, codegen places a
│        register-class result in rax/rdx and a memory-class result through the
│        hidden sret pointer saved in the prologue.
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
│      Stage 92: the for-node builder appends children via ast_add_child (the
│        children array is now dynamic, so the NULL-padded child slots are added
│        explicitly rather than written into a fixed array).
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
│        Stage 104: evaluator extended to relational (<, <=, >, >=),
│        equality (== !=), logical (&& ||), and ternary (?:). Additive/shift
│        precedence bug also fixed (3+1<<2 now evaluates to 16, not 7).
│        Non-constant or non-integer expressions → "case label expression
│        is not an integer constant expression". The folded value is stored as
│        an AST_INT_LITERAL.
│        (Codegen collects the labels into a per-switch `Vec` of
│        `SwitchLabel{node,label}`; the case/default count per switch is
│        unbounded since stage 95-12, when `MAX_SWITCH_LABELS` was removed.)
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
         Stage 91-01: whole-struct assignment via subscript (`arr[i] = f()`),
           dot (`obj.m = f()`), arrow (`p->m = f()`), and deref (`*p = f()`)
           targets all accept struct rvalues (six silent codegen bugs in these
           paths were fixed during stage-92 self-compilation).
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
                                                       Stage 99: "(type-name) {init}"
                                                         → build_compound_literal + parse_postfix_suffixes
                                                         (compound literal; takes priority over cast)
                                                         TOKEN_STRUCT and TOKEN_UNION now included in
                                                         the type-start detection check
                                                       "(type-name) <cast_expr>"
                                                         → regular cast: AST_CAST
                                                         type_name: optional const/volatile,
                                                           void, _Bool, int types,
                                                           signed/unsigned types, typedef
                                                           names, struct/union types
                                                         cast with (int[]) (omitted size) → error
                                                         sizeof(int[]) → error
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
                                                             AST_COMPOUND_LITERAL (stage 99 — lvalue)
                                                           sizeof forms:
                                                             "sizeof" <unary_expr>
                                                             "sizeof" "(" <type_name> ")"
                                                               sizeof(void) → error
                                                               sizeof(int[4][8]) multidim ok (stage 86)
                                                               sizeof a string literal → strlen+1 (stage 92 fix)
                                                               sizeof p->arr / s[i].arr member arrays now
                                                                 sized correctly (stage 92 fixes)
                                                             AST_SIZEOF_EXPR | AST_SIZEOF_TYPE
                                                           | no unary op → falls to postfix
                                                        └─► parse_postfix
                                                               Stage 99: tries compound literal detection
                                                               before parse_primary:
                                                                 saves lexer state before "("
                                                                 if (type-name) { → build_compound_literal
                                                                   + parse_postfix_suffixes
                                                                 if (type-name) but no { → restore state
                                                                   and fall through to parse_primary
                                                               Iterative suffix loop (left-assoc).
                                                               Stage 78/80: suffixes chain on ANY
                                                               postfix base, not just identifiers;
                                                               lvalue validity is enforced in codegen.
                                                               expr[i]    → AST_ARRAY_INDEX [base, index]
                                                                 base may be AST_VAR_REF, AST_DEREF,
                                                                 AST_ARRAY_INDEX, AST_MEMBER_ACCESS,
                                                                 AST_ARROW_ACCESS, or AST_COMPOUND_LITERAL
                                                                 (stage 99)
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
                                                                   string literal    → AST_STRING_LITERAL
                                                                     adjacent literals concatenated in a loop
                                                                       (stage 89); hex/octal escapes decoded
                                                                       into a StrBuf and interned in the lexer
                                                                       string pool (stage 95-08/09) — no length
                                                                       cap, embedded NUL bytes preserved;
                                                                       node->value is a const char * (stage 95-09);
                                                                       the string pool is a dynamic Vec (stage 95-05)
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
                                                                     struct/union value args marshalled per
                                                                       SysV class; a struct/union return value
                                                                       is materialised in caller scratch space
                                                                       (stage 91-01)
                                                                   indirect call from identifier
                                                                     AST_INDIRECT_CALL (callee AST_VAR_REF child)
                                                                     when name not in the function table
                                                                   identifier (variable reference) → AST_VAR_REF
                                                                   parenthesized expression
                                                                     "(" <expression> ")" → re-enters top

parse_initializer                          (used as initializer in declarations)
   scalar initializer: parse_assignment_expression
     Stage 99: scalar initializer may be an AST_COMPOUND_LITERAL
   brace-initializer: "{" { <initializer_element> [ "," ] } "}"
   AST_INITIALIZER_LIST, children: [initializer_element*]
   trailing comma allowed; empty "{}" → zero-child list (zero-fill); nested braces OK
   char-array struct members may be initialized from a string literal (stage 85)
   Stage 97: each element is dispatched through parse_initializer_element (see below)
└─► parse_initializer_element  (stage 97: each list element)
       "." <identifier> "=" <initializer>
         → AST_DESIGNATED_INIT (value = field name, byte_length = 0)
            child[0]: parse_initializer result
         chained designator detected (next token is "." or "[" after "="):
           → error "chained designators not yet supported"
       "[" <const_integer_expr> "]" "=" <initializer>
         → AST_DESIGNATED_INIT (value = NULL, byte_length = index)
            child[0]: parse_initializer result
         negative index → error "array designator index must be non-negative"
         chained designator detected: → error "chained designators not yet supported"
       otherwise → parse_initializer (unchanged path)
    ├─► eval_const_expr(parser, "array designator index")
    │     (stage 97: was eval_case_const_expr; stage 99: extended evaluator;
    │      stage 104: further extended to relational, equality, logical, ternary)
    └─► parse_initializer  (the designator's value)
└─► parse_assignment_expression  (scalar / non-brace elements)

build_compound_literal                     (stage 99: builds AST_COMPOUND_LITERAL)
   called from parse_cast (via parse_postfix) when "(type-name) {" is detected
   rejects void and function types: "error: invalid type for compound literal"
   struct/union: calls parse_initializer for the brace body
     sets decl_type=TYPE_STRUCT/TYPE_UNION, full_type=struct/union Type*
   array with explicit length N: calls parse_initializer
     sets decl_type=TYPE_ARRAY, full_type=type_array(elem, N)
   array with omitted first dimension (len==0 from parse_type_name):
     calls parse_initializer, then infer_compound_literal_array_length
     rebuilds full_type = type_array(elem, inferred_length)
   scalar: consumes "{", calls parse_assignment_expression, allows trailing
     comma, consumes "}"
     sets decl_type = scalar kind, full_type = NULL (or existing pointer type)
   all paths: node->byte_length = 0 (assigned by pre-scan before codegen)
              node->value = type label string in lexer pool ("int", "struct Point", "int[4]")
              child[0] = initializer
└─► infer_compound_literal_array_length    (stage 99)
       walks AST_INITIALIZER_LIST children
       tracks cursor; repositioned by AST_DESIGNATED_INIT (index) nodes
       returns max(highest designator index + 1, non-designated count)
└─► parse_initializer or parse_assignment_expression  (the brace body / scalar value)
```

### Constant-Expression Evaluator (stages 99–104)

The evaluator is a separate token-stream recursive descent within the parser,
entirely independent of the main expression-parser chain above. It is used only
where C99 requires compile-time integer constant expressions.

```
eval_const_expr(parser, context)                       ← top: enumerator values,
│  stage 99–103: called eval_const_bitwise_or           case labels, array designators,
│  stage 104: now calls eval_const_conditional          file-scope initializers
└─► eval_const_conditional                             (stage 104: new)
       <logical_or> [ "?" <const_expr> ":" <conditional> ]
       right-associative; true-branch re-enters eval_const_expr
    └─► eval_const_logical_or                          (stage 104: new)
           || loop; both operands eagerly evaluated; result 0 or 1
        └─► eval_const_logical_and                     (stage 104: new)
               && loop; both operands eagerly evaluated; result 0 or 1
            └─► eval_const_bitwise_or                  (stage 99)
                   | loop
                └─► eval_const_bitwise_xor             (stage 99)
                       ^ loop
                    └─► eval_const_bitwise_and         (stage 99)
                           & loop
                           stage 99–103: called eval_const_additive
                           stage 104: now calls eval_const_equality
                        └─► eval_const_equality        (stage 104: new)
                               == and != loop
                            └─► eval_const_relational  (stage 104: new)
                                   < <= > >= loop
                                └─► eval_const_shift   (stage 99; corrected stage 104)
                                       << and >> loop
                                       stage 99–103: called eval_const_multiplicative (bug)
                                       stage 104: now calls eval_const_additive (correct)
                                    └─► eval_const_additive  (stage 99; corrected stage 104)
                                           + and - loop
                                           stage 99–103: called eval_const_shift (bug)
                                           stage 104: now calls eval_const_multiplicative (correct)
                                        └─► eval_const_multiplicative  (stage 99)
                                               * / % loop; div-by-zero → PARSER_ERROR
                                            └─► eval_const_unary  (stage 99)
                                                   + - ~ ! (right-recursive)
                                                └─► eval_const_primary  (stage 99)
                                                       INT_LITERAL   → long_value
                                                       CHAR_LITERAL  → long_value
                                                       IDENTIFIER    → enum constant lookup
                                                         (not found → PARSER_ERROR "not an integer constant expression")
                                                       "sizeof" "(" type-name ")"
                                                         → type_size(t)  (stage 100)
                                                       "(" eval_const_expr ")"  (recursion)
                                                       other → PARSER_ERROR
```

The codegen-side evaluator `eval_const_init(ASTNode*, varname)` mirrors the
token-stream evaluator but walks an already-parsed AST (used for block-scope
static scalar initializers, stage 103). Stage 104 extends it to:
- `AST_BINARY_OP` with operators `<`, `<=`, `>`, `>=`, `==`, `!=` — return integer 0 or 1
- `AST_BINARY_OP` with operators `&&`, `||` — eagerly evaluate both sides, normalize to 0 or 1
- `AST_CONDITIONAL_EXPR` (3 children) — evaluate condition, then evaluate only the selected branch

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
     enumerator_list: <identifier> [ "=" <constant_integer_expression> ] { "," ... } [","]
     Stage 99: enumerator values call eval_const_expr(parser, "enumerator value")
       (was a literal-only check); enables 1<<0, BASE+5, ~0, (4*8), etc.
     Stage 104: enumerator values also support relational, equality, logical,
       and ternary: `enum { POSITIVE = 1 > 0, LIMIT = DEBUG ? 10 : 100 }`.
     registers constants in parser->enum_consts (next_val auto-increments);
     tags registered in parser->enum_tags with is_defined=1; returns type_int()
   "enum" <tag>   (reference without body)
     Stage 99: if tag not found, creates a forward-declaration entry
       (EnumTag { tag, is_defined=0 }) and returns type_int()
       (was: error "enum Tag is not defined")
     if tag found (defined or forward-declared) → type_int()

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
     Stage 91-01: SysV classification (register-class ≤16 bytes vs memory-class
       >16 bytes) is computed from this layout when a struct is passed/returned
       by value.
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
     by-value union parameters/returns classified as for structs (stage 91-01)
   "union" <tag>   (reference without body) → incomplete placeholder (size=0)
   └─► parse_type_specifier  (member base types)
   └─► parse_declarator      (member declarators)

parse_type_name                            (cast, sizeof(type), __builtin_va_arg type)
   [ "const" | "volatile" ] <type_specifier> <abstract_declarator>
   abstract_declarator: { "*" [ "const" | "volatile" ] } [ "[" [N] "]" { "[" N "]" } ]
     multidimensional abstract array declarators supported (stage 86)
     Stage 99: "[" "]" (omitted first dimension, length=0) accepted for compound
       literals; sizeof(T[]) and cast (T[])expr with length==0 → error
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
   Struct/union value parameter: full_type attached (stage 91-01) so codegen can
     classify and bind it (register-class via direct stores; memory-class via
     block copy from the stack into a private local)
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
- **Constant-expression evaluator (stages 77, 99–104):** a dedicated token-stream
  constant evaluator (`eval_const_expr`, now 15-level recursive descent after
  stage 104) folds integer/character literals, enum constants, `sizeof(type-name)`,
  and the full set of C99 compile-time integer operators into a single `long`
  value. The evaluator is called from case labels, enumerator values, array
  designator indices, file-scope scalar initializers, and (through `eval_const_init`
  on the already-parsed AST) block-scope static scalar initializers. A pre-existing
  additive/shift precedence bug (additive was calling shift as its subexpression,
  making shift bind tighter than additive) was fixed in stage 104.
- **`typedef enum` forward declarations (stage 99):** `parse_enum_specifier`
  now accepts an undefined `enum Tag` reference (no body) by creating a
  forward-declaration placeholder entry in `parser->enum_tags` and returning
  `type_int()`. This enables the idiomatic `typedef enum Status Status;`
  before `enum Status { OK, ERR };` pattern.
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
- **Block-scope `static` (stage 71; extended stages 101–104):** `parse_statement`
  accepts a leading `static`, parses the inner declaration, and tags the resulting
  `AST_DECLARATION` / `AST_DECL_LIST` children with `SC_STATIC`. `extern`
  remains illegal at block scope. Scalars/pointers originally accepted (stage 71);
  arrays/structs/unions now supported (stage 101); aggregate designators and 2D
  arrays (stage 102); scalar initializers now accept full constant expressions
  (stage 103); constant expressions extended to relational, equality, logical,
  and ternary (stage 104).
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
  consecutive `TOKEN_STRING_LITERAL` tokens, appending decoded bytes into a
  `StrBuf` and interning the result in the lexer string pool. Since stage 95-08/09
  there is no `MAX_NAME_LEN` length bound — arbitrarily long literals (and
  embedded NUL bytes) are supported, and `node->value` is a `const char *`.
- **Address-of member lvalues (stage 91):** unary `&` accepts `AST_MEMBER_ACCESS`
  and `AST_ARROW_ACCESS` operands; codegen reuses `emit_member_addr` /
  `emit_arrow_addr` and sets the result type to pointer-to-field.
- **Struct/union by value (stage 91-01):** parameters and return values follow
  the SysV AMD64 ABI — register-class (≤16 bytes) structs travel in
  `rax`/`rdx`/argument registers, memory-class (>16 bytes) structs through a
  hidden pointer. The parser attaches `full_type` to value parameters and return
  types; codegen adds `emit_struct_addr`/`emit_struct_copy` (block copy via
  `rep movsb`), `compute_struct_ret_bytes`/`claim_struct_ret_temp`, and a shared
  call-layout helper used by both call sites and prologues. Whole-struct
  assignment and decl-init from a struct rvalue (variable, call result, or copy)
  are supported.
- **Dynamic AST children (stage 92):** `ASTNode.children` is a lazily-allocated
  doubling array rather than a fixed 64-slot one. Before this, any child past
  the 64th was silently dropped — large translation units lost declarations
  (including `main`), and large blocks/switches lost statements. The `for`-node
  builder appends through `ast_add_child`.
- **Dynamic symbol tables (stages 95-04/05/06/07):** the parser and code
  generator no longer use fixed-capacity arrays for their symbol tables. The
  `Vec` container (stage 95-02) backs `enum_tags`, `union_tags`, `globals`,
  `enum_consts`, `struct_tags`, `typedefs`, and `funcs` in the parser, and
  `locals`, `globals`, `string_pool`, `break_stack`, `switch_stack`,
  `local_statics`, and `user_labels` in codegen, plus the struct/union
  field-parsing scratch buffer. Lookups/registrations use `vec_get`/`vec_push`.
  Three latent overflow bugs were fixed along the way (struct fields beyond 64
  silently dropped; unchecked `break_stack`; unguarded call-argument layout).
  Stage 95-12 finishes the series: the per-switch case/default label table in
  `SwitchCtx` becomes an embedded `Vec` of `SwitchLabel{node,label}` (init'd
  before the `SwitchCtx` is pushed, freed before it is popped, with the live
  top element re-fetched after the body so nested-switch reallocations cannot
  dangle it), and the preprocessor `#if` unary-operator run in `eval_cond_unary`
  becomes a `StrBuf` — the last unbounded fixed-capacity write in the tree.
- **Pointer-based name storage (stages 95-08/09/10/11):** identifier, tag, and
  label text is no longer copied into `char[MAX_NAME_LEN]` buffers. `Token`
  carries `const char *lexeme`/`value` with `size_t` lengths into a lexer-owned
  string pool (`lexer_store_bytes`, stage 95-08); `ASTNode.value`,
  `ParsedDeclarator.name`, every parser.h struct name/tag field, and every
  codegen name/label field are now `const char *` (stages 95-09/10/11). Names
  from AST/lexer storage are assigned by pointer; generated labels use
  `util_strdup`. The `MAX_NAME_LEN` cap no longer applies to any of them.
- **Per-file lifecycle (stage 96):** `parser_free()`, `codegen_free()`, and
  `reset_error_state()` are added to support multi-file compilation. The driver
  now accepts one or more source files; each is compiled through a fresh
  Lexer/Parser/CodeGen/AST cycle with full per-file teardown. String ownership
  in codegen is consolidated via `Vec owned_strings` and `codegen_intern()`.
  No parser functions, grammar rules, or AST node types changed.
- **Designated initializers (stage 97):** `parse_initializer`'s brace-list loop
  now calls `parse_initializer_element` for each element. That helper detects
  `.IDENT =` (member designator) or `[const_expr] =` (array index designator)
  and returns an `AST_DESIGNATED_INIT` node carrying either the field name
  (in `node->value`) or the integer index (in `node->byte_length`) plus the
  value initializer as `child[0]`. Chained designators (`.a.b`, `.arr[0]`) are
  detected before the `=` is consumed and rejected with a diagnostic. Array
  indices are evaluated by `eval_case_const_expr` (forward-declared so it is
  visible before its definition in the file). Codegen handles
  `AST_DESIGNATED_INIT` at four sites: local struct init (name lookup + cursor),
  local array init (two-phase zero-fill + cursor), global struct emit (slots[]
  map + declared-order emission), and global array emit (slots[] map + index
  emission). All new local arrays use fixed sizes (`MAX_STRUCT_FIELDS_DESIGNATED
  = 64`, `MAX_ARRAY_ELEMS_DESIGNATED = 1024`) since the compiler must self-host
  under its own pre-VLA restrictions.
- **Compound literals (stage 99):** `parse_cast` now detects `(type-name) {`
  and delegates to `build_compound_literal` + `parse_postfix_suffixes` instead
  of the regular cast path. `parse_postfix` also detects compound literals
  (saving/restoring lexer state) so that unary operators like `&` can apply to
  them. `build_compound_literal` handles struct/union (delegates to
  `parse_initializer`), array (with optional length inference via
  `infer_compound_literal_array_length`), and scalar (parses a braced
  assignment-expression). The `parse_type_name` abstract declarator now accepts
  `[]` as an omitted first dimension (length=0) specifically for compound
  literals. Stack slots for compound literals are pre-assigned by
  `scan_expr_compound_literals` (walks the AST before the prologue is emitted);
  the frame-size estimator `compute_compound_literal_bytes` contributes to
  `stack_size`. `AST_COMPOUND_LITERAL` at file scope → "compound literals at
  file scope are not yet supported" (still not supported).
- **File-scope constant expressions (stage 100):** file-scope scalar integer
  globals can now be initialized with the full `eval_const_expr` evaluator
  instead of literal-only. Supports arithmetic, bitwise, shift, unary, and
  `sizeof(type-name)`.
- **Block-scope static aggregates (stage 101–102):** block-scope `static` arrays,
  structs, and unions are now supported (previously only scalars/pointers).
  Codegen carries initializers via `LocalStaticVar.init_node`. Designated
  initializers (`[index]` and `.member`) work in local static arrays and global
  arrays (stage 102). Multidimensional static arrays are now fully supported
  (stage 102).
- **Block-scope static scalar constant expressions (stage 103):** block-scope
  `static` scalar variables now use `eval_const_init` to accept full
  constant-expression initializers instead of literal-only. Codegen-only change.
- **Complete constant-expression evaluator (stage 104):** both `eval_const_expr`
  (token-stream) and `eval_const_init` (AST-based) now support the complete C99
  integer constant expression operator set: relational (`<`, `<=`, `>`, `>=`),
  equality (`==`, `!=`), logical-AND (`&&`), logical-OR (`||`), and the
  conditional/ternary operator (`?:`). A pre-existing additive/shift precedence
  bug was also fixed: `eval_const_additive` previously called `eval_const_shift`
  (making shift bind tighter than additive), now correctly calls
  `eval_const_multiplicative`; `eval_const_shift` now correctly calls
  `eval_const_additive`. The `eval_const_conditional` ternary implementation is
  right-associative and uses lazy evaluation for the false branch. No parser,
  AST, or grammar changes.
- Typedef names are tracked in a scoped table (`scope_depth`); leaving a block
  removes typedefs registered at or below that depth.
- `parse_struct_specifier`/`parse_union_specifier` complete a forward-declaration
  placeholder in-place when a body is later provided.
- **Self-hosting (stages 92–104):** the compiler compiles itself. C0
  (gcc-built) produces C1, and C1 produces C2; all three pass the full
  1584-test suite, confirming bootstrap stability. The original bootstrap
  (stages 91-01/92) surfaced and fixed thirteen real defects (struct-by-value
  ABI, the silent AST-truncation bug, six struct-by-value/`sizeof` codegen bugs,
  duplicate-`extern` emission, literal/switch-label capacity limits, missing
  `global` directives, and `strtol` stubs). Stage 94 added the repeatable
  `build.sh --mode=self-host` C0→C1→C2 cycle, and the 95-xx storage-refactor
  stages each re-verified self-hosting. Stages 96–104 all passed self-hosting
  with no new issues. See `docs/self-compilation-report.md`.
- **Still parser-rejected (known gaps):** functions returning function pointers;
  pointer-to-array declarators (`(*p)[10]`); old-style (K&R) function
  definitions; chained designators (`.a.b`, `.arr[0]`); designated union init
  for non-first members; compound literals at file scope.
