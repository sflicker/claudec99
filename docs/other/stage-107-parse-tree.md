
# Full Grammar Parse Tree — Stage 107

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
fixed-capacity tables (`enum_tags`, `union_tags`, `globals`, `enum_consts`,
`struct_tags`, `typedefs`, `funcs`, `locals`, `string_pool`, `break_stack`,
`switch_stack`, struct-field scratch, `local_statics`) with `Vec`, removing
their hard caps; and stages 95-08 through 95-11 move all token, AST, parser,
and codegen name/tag/label text from `char[MAX_NAME_LEN]` buffers into
`const char *` pointers backed by a lexer-owned string pool — which removes the
old 255-byte string-literal cap entirely. Stage 95-12 closes the last two
fixed-capacity items: the preprocessor `#if` unary-operator buffer becomes a
`StrBuf` (removing the final unbounded fixed-capacity write) and the per-switch
case/default label table inside `SwitchCtx` becomes a `Vec` (removing the
`MAX_SWITCH_LABELS` cap).

**Stage 96 is a driver/lifecycle stage — no expression-grammar, parser function,
or AST node changes.** The following new lifecycle functions are added:

- `parser_free(Parser *)` (`src/parser.c`) — `vec_free`s the seven Vecs the
  parser owns (`funcs`, `globals`, `typedefs`, `enum_consts`, `enum_tags`,
  `struct_tags`, `union_tags`); element strings remain in lexer-owned storage.
- `codegen_free(CodeGen *)` (`src/codegen.c`) — frees all codegen-generated
  strings (via the new `Vec owned_strings` field and `codegen_intern()` helper)
  then `vec_free`s all eight Vecs in `CodeGen`.
- `reset_error_state()` (`src/util.c`) — zeroes `g_error_count`,
  `g_error_src_file/line/col`, and `g_error_jmp_valid` between files.

The driver (`src/compiler.c`) now accepts one or more positional source files,
calls `reset_error_state()` + `compile_one_file()` for each, and accumulates
the overall exit status; per-file teardown (parser_free / codegen_free /
lexer_free / ast_free / free(preprocessed)) ensures each file starts from a
clean slate. None of this changes which programs parse or how they are
represented in the AST.

**Stage 97 adds `AST_DESIGNATED_INIT` and `parse_initializer_element` — the
first brace-initializer grammar change since stage 32/43/44.** Two changes:

1. **New AST node `AST_DESIGNATED_INIT`** (`include/ast.h`): holds a designator
   + initializer value pair inside an `AST_INITIALIZER_LIST`. `node->value != NULL`
   → member designator (field name); `node->value == NULL` → array index
   designator (index stored as int in `node->byte_length`). `child_count` is
   always 1 (the value initializer, produced by `parse_initializer`).

2. **New helper `parse_initializer_element`** (`src/parser.c`): detects a
   leading `.` (member designator) or `[` (array index designator) before
   delegating to `parse_initializer`. Rejects chained designators (`.a.b`,
   `.arr[0]`) with "chained designators not yet supported". Array indices are
   evaluated by `eval_case_const_expr` (forward-declared before
   `parse_initializer` so it is visible here). `parse_initializer`'s brace-list
   loop now calls `parse_initializer_element` instead of itself for each element.

Codegen changes (Tasks 3–6) update `emit_local_struct_init`,
`codegen_statement` (local array), `emit_global_struct`, and
`codegen_emit_data` (global array) to handle `AST_DESIGNATED_INIT` children.
All four use a current-position cursor that a designator can reposition; global
contexts build a `slots[]` fixed-size array map before emitting in declared order
(no VLAs — the compiler must self-host under its own pre-VLA restrictions).

**Stage 99 extends the integer constant expression evaluator and enables
`typedef enum` forward references.** No new AST node types and no new
grammar productions. Two parser changes:

1. **`eval_const_expr` replaces `eval_case_const_*`** (`src/parser.c`): the
   three-function chain `eval_case_const_primary` / `eval_case_const_unary` /
   `eval_case_const_expr` (additive only) is replaced by a nine-level
   recursive descent evaluator (`eval_const_primary`, `eval_const_unary`,
   `eval_const_multiplicative`, `eval_const_shift`, `eval_const_additive`,
   `eval_const_bitwise_and`, `eval_const_bitwise_xor`, `eval_const_bitwise_or`,
   `eval_const_expr`). New operators: `*`, `/`, `%` (division-by-zero →
   `PARSER_ERROR`); `<<`, `>>`; `&`, `^`, `|`; unary `~`, `!`; parenthesized
   sub-expressions. A `const char *context` parameter is threaded through
   the chain so error messages are specific to the call site ("case label
   expression", "enumerator value", "array designator index").

2. **`parse_enum_specifier` accepts forward-declared tags** (`src/parser.c`):
   the no-body `enum Tag` path previously errored when `Tag` was not in the
   `enum_tags` table. Now, an unknown tag creates a forward-declaration entry
   (`EnumTag { tag, is_defined=0 }`) via `vec_push` and returns `type_int()`
   immediately. This enables the idiomatic `typedef enum Status Status;`
   pattern before the body is defined. When the body is subsequently parsed
   for the same tag, the existing logic that finds the entry and sets
   `is_defined=1` completes the forward reference without further change.
   The enumerator value assignment path is updated to call
   `eval_const_expr(parser, "enumerator value")` instead of the old
   literal-only check, enabling patterns like `FLAG = 1 << 0`,
   `STEP = BASE + 5`, and `ALL = ~0`.

---

**Stage 98 adds `AST_COMPOUND_LITERAL` and the compound literal expression
grammar — `(type-name) { initializer-list }`.** Three parser changes:

1. **New AST node `AST_COMPOUND_LITERAL`** (`include/ast.h`): represents a
   C99 compound literal (§6.5.2.5). Fields: `decl_type` = base kind,
   `full_type` = struct/union/array/pointer `Type*` (NULL for plain scalars),
   `byte_length` = 0 at parse time (pre-scan overwrites with rbp-relative
   stack offset), `value` = pre-formatted type label string stored in lexer
   pool, `children[0]` = `AST_INITIALIZER_LIST` (or bare expression for
   scalar). `child_count` is always 1.

2. **`parse_cast` extended** (`src/parser.c`): after parsing `(type-name)`,
   if the next token is `TOKEN_LBRACE`, the type is a compound literal rather
   than a cast. `TOKEN_STRUCT` and `TOKEN_UNION` added to the type-start
   detection check. Compound literal is built by `build_compound_literal` and
   then `parse_postfix_suffixes` is applied, enabling `.field` / `[idx]`
   chaining. A void or function type in compound literal context → "error:
   invalid type for compound literal". An omitted-size array (`(int[])`) is
   valid for compound literals (the length is inferred from the initializer)
   but invalid for casts → "error: array type in cast has omitted size".
   `sizeof(int[])` → "error: sizeof applied to incomplete array type".

3. **`parse_postfix` extended** (`src/parser.c`): tries compound literal
   detection (`(type-name){`) before falling back to `parse_primary`. Saves
   lexer state before the `(`; if `(type-name)` is found but not followed by
   `{`, restores the saved state and falls through to `parse_primary` (which
   handles ordinary parenthesized expressions). This allows unary operators
   like `&` and `-` to directly apply to compound literals.

Additional parser helpers added:

- **`build_compound_literal(Parser *, Type *)`**: creates `AST_COMPOUND_LITERAL`
  from a parsed type and the following brace-initializer. Handles struct/union
  (delegates to `parse_initializer`), explicit-size array (delegates to
  `parse_initializer`), omitted-size array (delegates to `parse_initializer`
  then calls `infer_compound_literal_array_length`), and scalar (manually
  parses `{` assignment-expression `}`). Stores a type label in the lexer
  pool for `print_ast` display.
- **`infer_compound_literal_array_length(ASTNode *list)`**: walks the
  initializer list, tracks a cursor (repositioned by any `[N]` designator),
  and returns `max(highest designator index + 1, non-designated count)`.
- **`parse_postfix_suffixes(Parser *, ASTNode *base)`**: extracted from
  `parse_postfix`; applies any trailing `[expr]`, `.field`, `->field`,
  `(args)`, `++`, `--` suffixes to `base`. Called by both `parse_postfix`
  and `build_compound_literal` (via `parse_cast`) so compound literals
  participate in postfix chaining.

`parse_type_name` updated: `[]` (omitted first dimension at index position 0)
is now accepted in the array-suffix loop for compound literals; inner dimensions
must still be explicit.

`parse_unary` updated: the lvalue check for `&` now also accepts
`AST_COMPOUND_LITERAL` operands (compound literals are C99 lvalues).

Global initializer updated: uses `parse_assignment_expression` instead of
`parse_primary` for the non-brace path, so compound literals are fully parsed
before the constant-only check. `AST_COMPOUND_LITERAL` in that position →
"error: compound literals at file scope are not yet supported" (more specific
than the generic "non-constant initializer" message).

---

**Stage 100 adds file-scope constant-expression evaluators.** No new parser
functions, AST nodes, or grammar productions:

1. **`eval_const_primary` extended** (`src/parser.c`): now handles `TOKEN_SIZEOF`.
   `sizeof(type-name)` is now valid in constant expressions, enabling patterns
   like `static int sizes[3] = {sizeof(int), sizeof(long), sizeof(char)}`.

2. **`parse_type_specifier` extended** (`src/parser.c`): `TOKEN_ENUM` added to
   the type-start condition, so `sizeof(enum Color)` works in constant contexts.

3. **File-scope initializer path** (`src/parser.c`, `parse_external_declaration`):
   integer-typed globals now call `eval_const_expr` for initializer validation
   instead of literal-only checks, enabling patterns like
   `static int base = 1 << 4; static int mask = BASE - 1;`.

---

**Stage 101 adds block-scope static aggregate support (parser-side).** No parser
changes; codegen only:

1. **`LocalStaticVar` struct extended** (`include/codegen.h`): gains an `init_node`
   field (previously unused). Block-scope static arrays and structs can now carry
   aggregate initializers (AST_INITIALIZER_LIST children).

---

**Stage 102 extends designated-initializer support for block-scope statics.**
No parser changes; codegen array designator handling updated.

---

**Stage 103 adds `eval_const_init` for block-scope static scalar initializers.**
Codegen-side only; no parser changes. A static helper in `src/codegen.c` evaluates
constant expressions at code-generation time for block-scope static scalars,
mirroring the parser-side `eval_const_expr` with `AST_INT_LITERAL`,
`AST_CHAR_LITERAL`, `AST_SIZEOF_TYPE`, unary and binary operators.

---

**Stage 104 completes the constant-expression evaluator.** Parser-side fixes and
extensions:

1. **Fixed precedence bug** (`src/parser.c`): `eval_const_additive` and
   `eval_const_shift` had inverted call order — corrected to match C operator
   precedence (shift binds tighter than addition).

2. **Extended evaluation chain** (`src/parser.c`): the original 9-level chain
   is extended to 13 levels by adding:
   - `eval_const_relational` (< <= > >=)
   - `eval_const_equality` (== !=)
   - `eval_const_logical_and` (&&)
   - `eval_const_logical_or` (||)
   - `eval_const_conditional` (ternary ?:)

   The full chain now reads:
   ```
   eval_const_expr
     → eval_const_logical_or
       → eval_const_logical_and
         → eval_const_equality
           → eval_const_relational
             → eval_const_bitwise_or
               → eval_const_bitwise_xor
                 → eval_const_bitwise_and
                   → eval_const_additive
                     → eval_const_shift
                       → eval_const_multiplicative
                         → eval_const_unary
                           → eval_const_primary
   ```

3. **Codegen-side `eval_const_init`** extended likewise with all 13 operator levels.

---

**Stage 105 completes the C99 preprocessor.** Parser-side additions:

1. **`#pragma` directive** (`src/preprocessor.c`): `preprocess_file` recognizes
   `#pragma` directives. Unknown pragmas are silently ignored; `#pragma once`
   calls `macro_table_add_once` to mark the file as once-included.

2. **`#line` directive** (`src/preprocessor.c`): parses required digit sequence
   (line number), updates `current_line`; optionally parses a quoted filename,
   stores in `current_file_override`. `__FILE__` expansion uses
   `current_file_override` when non-NULL.

3. **Null directive** (`src/preprocessor.c`): a bare `#` followed by
   whitespace+newline is recognized and continues parsing.

4. **`_Pragma` operator** (`src/preprocessor.c`, identifier expansion):
   recognized as `_Pragma("string-literal")`. The string `"once"` triggers
   `macro_table_add_once`; anything else is silently ignored.

5. **`__func__` support** (`src/parser.c`):
   - `Parser` struct gains `current_func_name` field (after `current_func_is_variadic`).
     Set at function-definition entry; NULL when not in a function.
   - `parse_primary` extended: handles `__func__` before `__builtin_va_*`.
     Error when `current_func_name == NULL` ("used outside of a function body").
     Synthesizes an `AST_STRING_LITERAL` with the function name via
     `lexer_store_bytes(fname, len)`, setting `byte_length=len` and
     `full_type=type_array(type_char(), len+1)`.

---

**Stage 106 adds the `restrict` qualifier.** Parser-side keyword and qualifier
handling:

1. **`TOKEN_RESTRICT` added** (`include/token.h`, `src/lexer.c`): new keyword
   token after `TOKEN_VOLATILE`. Lexer maps the `"restrict"` keyword to this token.

2. **All pointer-qualifier positions consume `TOKEN_RESTRICT`** (parse-and-ignore,
   same pattern as `volatile`):
   - `parse_declarator` leading-star loop: changed from `if` to `while` over
     `TOKEN_CONST | TOKEN_VOLATILE | TOKEN_RESTRICT`
   - `parse_type_name` abstract-declarator star loop: added `TOKEN_RESTRICT`
   - `parse_declarator` fp-param-types loop: added `TOKEN_RESTRICT`
   - `parse_parameter_declaration` leading-qualifier check: added `TOKEN_RESTRICT`
   - `parse_parameter_declaration` pre-consume stars loop: added inner
     qualifier-consumption loop after each `*` to handle `restrict` between
     `*` and identifier name

3. **No semantic or codegen effect:** `restrict` has no representation in the
   Type system; it is parse-and-ignore like `volatile` at this stage.

---

**Stage 107 adds the `inline` function-specifier.** Minimal parser change and
one preprocessor fix:

1. **`TOKEN_INLINE` added** (`include/token.h`, `src/lexer.c`): new keyword token
   after `TOKEN_RESTRICT`. Lexer maps the `"inline"` keyword to this token.
   `token_type_name()` returns `"'inline'"`.

2. **`parse_declaration_specifiers` consumes `TOKEN_INLINE`** (parse-and-ignore,
   same pattern as `volatile` and `restrict`):
   - A new `else if` branch in the `while (1)` qualifier loop advances past
     `TOKEN_INLINE` and continues without setting any field in `DeclSpecResult`.
   - Position: after the `TOKEN_VOLATILE` branch and before the
     `TOKEN_EXTERN`/`TOKEN_STATIC`/`TOKEN_TYPEDEF` branch.
   - No semantic or codegen effect: `inline` is a C99 §6.7.4 function-specifier;
     ClaudeC99 never inlines functions, so parse-and-ignore is correct.
   - Unlike `restrict`, `inline` never appears after `*` inside a declarator, so
     no changes to pointer-qualifier loops are needed.

3. **Preprocessor fix: `__FILE__`/`__LINE__` in macro bodies** (`src/preprocessor.c`):
   - Two static globals `g_expand_source_path` and `g_expand_current_line` are
     added at file scope (forward-declared after `expand_macros_text`'s prototype).
   - `preprocess_internal` sets these globals immediately before each call to
     `expand_macros_text` (argument pre-expansion and body rescan) so the
     expansion of function-like macro bodies can resolve `__FILE__`/`__LINE__`
     to the invocation-site file and line.
   - `expand_macros_text` now checks for `__FILE__` (emits quoted path) and
     `__LINE__` (emits decimal integer) before the `macro_find` lookup, handling
     them before the generic identifier dispatch.
   - This fix is required for `<assert.h>`'s assert macro, which uses both
     predefined macros in its body.

4. **`AST_BUILTIN_VA_COPY` codegen** (`src/codegen.c`, codegen only; no AST or
   parser changes):
   - The combined `va_end`/`va_copy` no-op branch is split.
   - `AST_BUILTIN_VA_COPY` now emits three 8-byte register moves via `rax` to
     copy the 24-byte SysV AMD64 `va_list` struct from `src` to `dst`.
   - Both `dst` and `src` must be local variables resolved via `codegen_find_var`.

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
       Stage 105: while parsing a definition body, current_func_name is set
         so __func__ can be synthesized correctly.
       Stage 91-01: a struct/union *return type* now carries its full_type so
         codegen can classify it (register-class ≤16 bytes vs memory-class
         returned through a hidden pointer); the type was previously NULL.
       void object declaration → compile-time error
       Stage 100: file-scope integer initializers now call eval_const_expr
         (stage 100, not literal-only check).
       Stage 99: file-scope initializer uses parse_assignment_expression (not
         parse_primary) for the non-brace path; AST_COMPOUND_LITERAL in a
         file-scope initializer → "compound literals at file scope not yet supported"
    ├─► parse_declaration_specifiers
    │      Collects optional const/volatile/restrict/inline qualifiers + optional
    │      storage class + one type specifier, in any order.
    │      type qualifier: "const" | "volatile" | "restrict"
    │      function-specifier: "inline"  (parse-and-ignore — stage 107)
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
│      (Block-scope static arrays and structs now supported in codegen — stage 101)
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
├─► [ "const" | "volatile" | "restrict" ] ( "void" | "_Bool" | "char" | "short" | "int" |
│   "long" | "signed" | "unsigned" | "enum" | "struct" | "union" |
│   <typedef-name> )
│      →  declaration (scalar, pointer, array, struct, union, enum, function-pointer)
│      [ qualifiers ] <type_specifier> <init_declarator_list> ";"
│
│      Leading "const": sets local_is_const; applies to variable when no pointer depth.
│      Leading "volatile": tracked on the type (stage 82-04; no codegen effect yet).
│      Leading "restrict": parse-and-ignore (stage 106; no semantic effect).
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
│        Stage 104: evaluator extended to relational (< <= > >=), equality (== !=),
│        logical-and (&&), logical-or (||), and ternary (?:).
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
                                                         type_name: optional const/volatile/restrict,
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
                                                                   __func__ (stage 105)
                                                                     matched before __builtin_va_* and enum/identifier;
                                                                     error when used outside a function body
                                                                       (current_func_name == NULL)
                                                                     synthesizes a read-only AST_STRING_LITERAL
                                                                       for the enclosing function name
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
                                                                       Stage 107: codegen now emits three
                                                                         8-byte rax moves (24-byte va_list copy)
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
    │      stage 104: full 13-level chain)
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
       Stage 104: evaluator extended to relational, equality, logical, ternary operators.
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
   [ "const" | "volatile" | "restrict" ] <type_specifier> <abstract_declarator>
   abstract_declarator: { "*" [ "const" | "volatile" | "restrict" ] } [ "[" [N] "]" { "[" N "]" } ]
     multidimensional abstract array declarators supported (stage 86)
     Stage 99: "[" "]" (omitted first dimension, length=0) accepted for compound
       literals; sizeof(T[]) and cast (T[])expr with length==0 → error
   accepts: void, _Bool, scalar keywords, signed/unsigned types, typedef names,
            "struct"/"union" tags, "enum" tags
   leading const/volatile/restrict applied to the base type via type_const_copy /
     type_volatile_copy (stage 82-03/04); restrict is parse-and-ignore (stage 106)
└─► parse_type_specifier

parse_declarator                           (declarations and parameters)
   Leading stars with optional per-pointer const/volatile/restrict (stage 66/82-04/106):
     { "*" [ "const" | "volatile" | "restrict" ] }
     the last "const" after a star sets ParsedDeclarator.pointer_is_const
     the last "restrict" after a star is parse-and-ignore (stage 106)
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
       each star may be followed by const/volatile/restrict (stage 106)
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

parse_declaration_specifiers               (file-scope and block-scope)
   Collects optional const/volatile/restrict/inline + one storage class +
   one type specifier, in any order.
   storage class: "extern" | "static" | "typedef"
   type qualifier: "const" | "volatile" | "restrict"
   function-specifier: "inline"  (parse-and-ignore — stage 107)
   Errors: duplicate storage class, duplicate type specifier, missing type specifier
   Returns DeclSpecResult {sc, base_kind, base_type, is_const, is_volatile}
└─► parse_type_specifier

parse_parameter_declaration                (inside parameter lists)
   [ "const" | "volatile" | "restrict" ] <type_specifier> [ <declarator> | { "*" } ]
   Named parameter: AST_PARAM (name in node->value)
   Unnamed scalar: next token is "," or ")"  → AST_PARAM (empty name)
   Unnamed pointer: leading "*" stars consumed (with inter-star qualifiers),
     then "," or ")" → AST_PARAM (empty name, decl_type=TYPE_POINTER)
   Array parameter (named or unnamed): adjusted to pointer-to-element
     (C99 §6.7.5.3p7; stages 75-02 / 75-04 — covers array-typedef params too)
   Struct/union value parameter: full_type attached (stage 91-01) so codegen can
     classify and bind it (register-class via direct stores; memory-class via
     block copy from the stack into a private local)
   function-pointer parameter: decl_type=TYPE_POINTER, full_type=fp chain
   leading const/volatile/restrict consumed (const/volatile tracked on type;
     restrict is parse-and-ignore — stage 106)
   per-pointer qualifiers: const/volatile/restrict between "*" and identifier,
     and between "*" tokens in pointer chains (stage 106)
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
- **Constant-expression `case` labels (stages 77, 99, 104):** a dedicated
  constant evaluator (`eval_const_expr`, 13-level recursive descent) folds
  integer/character literals, enum constants, and the full set of C99 compile-time
  integer operators into a single `long` value. The chain is:
  ```
  eval_const_expr → eval_const_logical_or → eval_const_logical_and
    → eval_const_equality → eval_const_relational → eval_const_bitwise_or
    → eval_const_bitwise_xor → eval_const_bitwise_and → eval_const_additive
    → eval_const_shift → eval_const_multiplicative → eval_const_unary
    → eval_const_primary
  ```
  Supports all operators: `* / % << >> + - & ^ | < <= > >= == != && || ?:`,
  unary `~ !`, and parenthesized sub-expressions. Division or modulo by zero
  → `PARSER_ERROR`. Examples: `case 1 << 2:`, `case ENUM_CONST * 2:`,
  `case x > y ? 1 : 0:` are now accepted. The same evaluator is used by
  `parse_initializer_element` for array designator indices (stage 97),
  `parse_enum_specifier` for enumerator values (stage 99), and
  `parse_external_declaration` for file-scope integer initializers (stage 100).
- **File-scope constant-expression initializers (stage 100):** file-scope
  integer-typed globals now call `eval_const_expr` for initializer validation,
  enabling patterns like `static int base = 1 << 4`.
- **`typedef enum` forward declarations (stage 99):** `parse_enum_specifier`
  now accepts an undefined `enum Tag` reference (no body) by creating a
  forward-declaration placeholder entry in `parser->enum_tags` and returning
  `type_int()`. This enables the idiomatic `typedef enum Status Status;`
  before `enum Status { OK, ERR };` pattern.
- **Block-scope static aggregates (stage 101):** block-scope `static` arrays
  and structs are now supported in codegen. The parser marks them as
  `SC_STATIC` and codegen emits them to `.bss/.data`; the `LocalStaticVar`
  struct carries an `init_node` field for aggregate initializers.
- **Designated initializers (stage 97):** `parse_initializer`'s brace-list loop
  now calls `parse_initializer_element` for each element. That helper detects
  `.IDENT =` (member designator) or `[const_expr] =` (array index designator)
  and returns an `AST_DESIGNATED_INIT` node carrying either the field name
  (in `node->value`) or the integer index (in `node->byte_length`) plus the
  value initializer as `child[0]`. Chained designators (`.a.b`, `.arr[0]`) are
  detected before the `=` is consumed and rejected with a diagnostic. Array
  indices are evaluated by `eval_const_expr` (stage 104 extension included).
  Codegen handles `AST_DESIGNATED_INIT` at four sites: local struct init
  (name lookup + cursor), local array init (two-phase zero-fill + cursor),
  global struct emit (slots[] map + declared-order emission), and global array
  emit (slots[] map + index emission). All new local arrays use fixed sizes
  (`MAX_STRUCT_FIELDS_DESIGNATED = 64`, `MAX_ARRAY_ELEMS_DESIGNATED = 1024`)
  since the compiler must self-host under its own pre-VLA restrictions.
- **Compound literals (stage 98):** `parse_cast` now detects `(type-name) {`
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
- **C99 preprocessor (stage 105):** `#pragma once` marks the file as once-only;
  `#pragma` <unknown> is silently ignored. `#line` directive sets line numbering
  and optional file override. `_Pragma("once")` macro operator works like
  `#pragma once`. `__func__` in `parse_primary` synthesizes a read-only string
  literal for the current function name (error at file scope). Null directives
  (bare `#` + newline) are recognized and ignored.
- **`restrict` qualifier (stage 106):** the `restrict` keyword is recognized
  in all pointer-qualifier positions: leading qualifiers in declarators, per-pointer
  qualifiers (after `*`), and abstract declarators. It is parse-and-ignore with no
  semantic or codegen effect (reserved for future optimization).
- **`inline` function-specifier (stage 107):** the `inline` keyword is recognized
  in `parse_declaration_specifiers` and consumed without effect — the same
  parse-and-ignore pattern used for `volatile` and `restrict`. Unlike `restrict`,
  `inline` never appears after `*` in a declarator, so no pointer-qualifier
  loop changes are needed. No codegen effect: ClaudeC99 does not perform inlining.
- **`__FILE__`/`__LINE__` in macro bodies (stage 107):** `expand_macros_text`
  (the helper used for argument pre-expansion and rescan during function-like
  macro expansion) now handles `__FILE__` and `__LINE__` using file-scope
  static globals `g_expand_source_path` and `g_expand_current_line`, set by
  `preprocess_internal` before each call. This enables the `<assert.h>` macro
  to correctly report the invocation-site file and line number.
- **`va_copy` codegen (stage 107):** `AST_BUILTIN_VA_COPY` is no longer a
  no-op; codegen emits three 8-byte `rax` moves to copy the 24-byte SysV AMD64
  `va_list` struct from the source to the destination local variable.
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
- **Self-hosting (stages 92–107):** the compiler compiles itself. C0
  (gcc-built) produces C1, and C1 produces C2; all three pass the full
  1615-test suite, confirming bootstrap stability. The original bootstrap
  (stages 91-01/92) surfaced and fixed thirteen real defects (struct-by-value
  ABI, the silent AST-truncation bug, six struct-by-value/`sizeof` codegen bugs,
  duplicate-`extern` emission, literal/switch-label capacity limits, missing
  `global` directives, and `strtol` stubs). Stage 94 added the repeatable
  `build.sh --mode=self-host` C0→C1→C2 cycle, and the 95-xx storage-refactor
  stages each re-verified self-hosting. Stages 96 through 107 all passed
  self-hosting with no new issues. See `docs/self-compilation-report.md`.
- Typedef names are tracked in a scoped table (`scope_depth`); leaving a block
  removes typedefs registered at or below that depth.
- `parse_struct_specifier`/`parse_union_specifier` complete a forward-declaration
  placeholder in-place when a body is later provided.
- **Still parser-rejected (known gaps):** functions returning function pointers;
  pointer-to-array declarators (`(*p)[10]`); old-style (K&R) function
  definitions; chained designators (`.a.b`, `.arr[0]`); designated union init
  for non-first members; compound literals at file scope.
