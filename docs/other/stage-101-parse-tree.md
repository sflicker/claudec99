# Full Grammar Parse Tree — Stage 101

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
   evaluated by `eval_const_expr` (forward-declared before
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
   pattern before the body is defined.

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
   chaining.

3. **`parse_postfix` extended** (`src/parser.c`): tries compound literal
   detection (`(type-name){`) before falling back to `parse_primary`. Saves
   lexer state before the `(`; if `(type-name)` is found but not followed by
   `{`, restores the saved state and falls through to `parse_primary`.

---

**Stage 100 wires `eval_const_expr` into file-scope integer initializers and
extends `eval_const_primary` with `sizeof(type-name)`.** No new AST node types.
Three parser changes:

1. **`eval_const_primary` extended with `sizeof(type-name)`** (`src/parser.c`):
   the `TOKEN_SIZEOF` branch is added before the fall-through error. Logic:
   consume `sizeof`, require `TOKEN_LPAREN`, consume `(`, check if current token
   starts a type-name, call `parse_type_name(parser)` → `Type *t`, check
   `t->kind == TYPE_VOID` (reject bare `sizeof(void)`) and incomplete array,
   then `parser_expect(TOKEN_RPAREN)` and `return (long)type_size(t)`. This
   enables `sizeof(int) * 256`, `sizeof(struct Pair)`, `sizeof(void *)`, and
   `sizeof(enum Color)` in constant-expression contexts.

2. **`parse_external_declaration` first-declarator path updated**: non-brace
   initializer for integer-typed globals now calls `eval_const_expr`; pointer/
   struct/union retain the original path.

3. **`parse_external_declaration` multi-declarator path updated**: same
   `eval_const_expr` wiring for non-pointer types.

---

**Stage 101 is a codegen-only stage — no parser, AST, or grammar changes.**
The parser already accepted `static` at block scope with any type (including
`TYPE_ARRAY`, `TYPE_STRUCT`, `TYPE_UNION`) since stage 71. The only changes
are in `src/codegen.c` and `include/codegen.h`:

1. **`LocalStaticVar` extended** (`include/codegen.h`): added `ASTNode *init_node`
   field to carry the brace-list (`AST_INITIALIZER_LIST`) or string-literal
   (`AST_STRING_LITERAL`) initializer for aggregate statics. Set to `NULL`
   for scalar statics and for uninitialized (`.bss`) entries.

2. **`codegen_statement` SC_STATIC arm** (`src/codegen.c`): guard that
   called `compile_error("static local arrays, structs and unions are not yet
   supported")` for `TYPE_ARRAY`/`TYPE_STRUCT`/`TYPE_UNION` is removed.
   New early-return blocks handle:
   - **Array statics**: validates initializer (must be `AST_INITIALIZER_LIST`
     or `AST_STRING_LITERAL` for `char` arrays), generates `Lstatic_func_N`
     label, registers `LocalVar` with `is_static=1`, pushes `LocalStaticVar`
     with `init_node` set, and returns.
   - **Struct/union statics**: validates non-zero type size (incomplete-type
     check), validates brace-list initializer if present, same label/
     registration pattern, and returns.
   - Scalar fallthrough then proceeds exactly as before.

3. **`codegen_emit_local_statics()`** (`src/codegen.c`): `.data` loop
   extended with branches before the scalar fallthrough:
   - `TYPE_ARRAY` + `AST_STRING_LITERAL` → inline `db` bytes (char array init).
   - `TYPE_ARRAY` + `AST_INITIALIZER_LIST` → `slots[]` map pattern (reusing
     `MAX_ARRAY_ELEMS_DESIGNATED`), emitting each slot with the appropriate
     NASM directive; designated initializers in this context are currently
     rejected.
   - `TYPE_STRUCT` → calls `emit_global_struct(cg, sv->full_type, sv->init_node)`.
   - `TYPE_UNION` → inline first-member logic.
   `.bss` loop extended: `TYPE_ARRAY` → `resx length`; `TYPE_STRUCT`/
   `TYPE_UNION` → `resb total_size`; scalar unchanged.

4. **`codegen_expression` VAR_REF `TYPE_ARRAY` branch**: added `is_static`
   guard → `lea rax, [rel label]` for static arrays (previously always
   emitted `lea rax, [rbp - offset]`).

5. **`emit_array_index_addr()`**: added `is_static` guard →
   `lea rax, [rel label]`; removed stale comment that stated "Array statics
   are rejected at declaration time".

6. **`emit_member_addr()` local-struct branch**: added `is_static` guard →
   `lea rax, [rel label + offset]` or `lea rax, [rel label]` (when offset is
   0) for static structs. (`emit_struct_addr()` already handled `is_static`
   at lines ~1302–1305 — unchanged.)

---

## 1. Program Structure

```
parse_translation_unit
  └─► loop over parse_external_declaration
        ├─► parse_specifiers (storage class, type, qualifiers)
        ├─► parse_declarator (name, pointer depth, array/function suffix)
        ├─► if function body → parse_function_definition
        │     └─► parse_block (compound statement)
        ├─► if '=' non-brace → [Stage 100] eval_const_expr (integer scalars)
        │                    │  parse_assignment_expression (pointer/struct/union)
        │                    │  then literal-only check
        ├─► if '=' '{' → parse_initializer (structs, unions, arrays)
        ├─► if ',' → loop multi-declarator
        │             if '=' non-pointer → [Stage 100] eval_const_expr
        │             if '=' pointer     → parse_primary + literal check
        └─► produce: AST_DECLARATION / AST_DECL_LIST / AST_FUNCTION_DECL
```

## 2. Statements

```
parse_statement
  ├─► TOKEN_LBRACE     → parse_block
  │     └─► loop: parse_declaration | parse_statement
  ├─► TOKEN_IF         → parse_if
  │     └─► parse_expression, parse_statement [, parse_statement]
  ├─► TOKEN_WHILE      → parse_while
  │     └─► parse_expression, parse_statement
  ├─► TOKEN_DO         → parse_do_while
  │     └─► parse_statement, parse_expression
  ├─► TOKEN_FOR        → parse_for
  │     ├─► [C99 init-decl] parse_declaration | parse_expression
  │     ├─► [cond] parse_expression
  │     ├─► [post] parse_expression
  │     └─► parse_statement
  ├─► TOKEN_SWITCH     → parse_switch
  │     └─► parse_expression, parse_block (with case/default handling)
  │           case constant evaluated by eval_const_expr [Stage 99]
  ├─► TOKEN_RETURN     → parse_return
  │     └─► parse_expression (optional)
  ├─► TOKEN_BREAK      → AST_BREAK
  ├─► TOKEN_CONTINUE   → AST_CONTINUE
  ├─► TOKEN_GOTO       → AST_GOTO (label name)
  ├─► TOKEN_IDENTIFIER + ':' → AST_LABEL (user label)
  └─► expression statement → parse_expression + ';'
```

## 3. Expressions

```
parse_expression          (comma operator: lowest precedence)
  └─► parse_assignment_expression
        ├─► parse_conditional_expression
        │     └─► parse_logical_or_expression
        │           └─► parse_logical_and_expression
        │                 └─► parse_bitwise_or_expression
        │                       └─► parse_bitwise_xor_expression
        │                             └─► parse_bitwise_and_expression
        │                                   └─► parse_equality_expression
        │                                         └─► parse_relational_expression
        │                                               └─► parse_shift_expression
        │                                                     └─► parse_additive_expression
        │                                                           └─► parse_multiplicative_expression
        │                                                                 └─► parse_cast
        │                                                                       ├─► (type-name) expr  → AST_CAST
        │                                                                       ├─► (type-name) { }  → [Stg 98] compound literal
        │                                                                       └─► parse_unary
        │                                                                             ├─► '-'/'+'/'~'/'!' → unary arithmetic
        │                                                                             ├─► '&' → AST_ADDR_OF
        │                                                                             ├─► '*' → AST_DEREF
        │                                                                             ├─► '++'/`--` (prefix) → AST_PREFIX_INC_DEC
        │                                                                             ├─► sizeof(type) → AST_SIZEOF_TYPE [void * fixed Stg 100]
        │                                                                             ├─► sizeof expr → AST_SIZEOF_EXPR
        │                                                                             └─► parse_postfix
        │                                                                                   ├─► [Stg 98] (type-name){} → compound literal
        │                                                                                   └─► parse_primary
        │                                                                                         ├─► INT_LITERAL, CHAR_LITERAL, STRING_LITERAL
        │                                                                                         ├─► IDENTIFIER (var ref / func call / enum)
        │                                                                                         ├─► '(' expression ')'
        │                                                                                         └─► sizeof(type/expr)
        └─► unary_expr assign_op assign_expr → assignment

```

### Constant-Expression Evaluator (stage 99 / stage 100)

```
eval_const_expr           [entry point; "case label", "enumerator value",
  └─► eval_const_bitwise_or        "array designator index", "file-scope initializer"]
        └─► eval_const_bitwise_xor
              └─► eval_const_bitwise_and
                    └─► eval_const_additive
                          └─► eval_const_shift
                                └─► eval_const_multiplicative
                                      └─► eval_const_unary
                                            └─► eval_const_primary
                                                  ├─► TOKEN_INT_LITERAL   → long_value
                                                  ├─► TOKEN_CHAR_LITERAL  → long_value
                                                  ├─► TOKEN_IDENTIFIER    → enum-const lookup
                                                  ├─► TOKEN_LPAREN        → eval_const_expr + ')'
                                                  └─► TOKEN_SIZEOF        → [Stage 100]
                                                        consume '(' + type-start check
                                                        → parse_type_name → type_size(t)
                                                        (TOKEN_VOID/TOKEN_ENUM added to type-start)
```

Callers:
- `parse_switch`: `case` label constants — context: "case label expression"
- `parse_enum_specifier`: enumerator values — context: "enumerator value"
- `parse_initializer_element`: array designator indices — context: "array designator index"
- `parse_external_declaration` [Stage 100]: file-scope integer initializers — context: "file-scope initializer"

## 4. Types and Declarators

```
parse_type_specifier
  ├─► TOKEN_INT/CHAR/SHORT/LONG → scalar base type
  ├─► TOKEN_UNSIGNED/SIGNED     → unsigned/signed modifier
  ├─► TOKEN_LONG TOKEN_LONG     → long long
  ├─► TOKEN_BOOL                → _Bool type
  ├─► TOKEN_VOID                → void
  ├─► TOKEN_STRUCT/TOKEN_UNION  → parse_struct_or_union_specifier
  ├─► TOKEN_ENUM                → parse_enum_specifier [fwd-refs: Stage 99]
  └─► TOKEN_IDENTIFIER (typedef)→ typedef lookup → stored type

parse_type_name             (abstract declarator; used in casts, sizeof, compound literals)
  ├─► parse_type_specifier
  ├─► zero or more '*' (with optional const/volatile qualifiers)
  └─► optional '[N]' array suffix (omitted first dim allowed for Stage 98 compound literals)

parse_declarator            (named declarator; used in declarations)
  ├─► zero or more '*' (pointer prefix)
  ├─► optional '(' → parse_declarator (parenthesized grouping)
  ├─► IDENTIFIER (the declared name)
  └─► optional '[N]' or '(params)' suffix

parse_function_parameters
  └─► loop: parse_type_specifier + parse_declarator (per parameter)

parse_initializer
  ├─► '{' → parse_initializer_list
  │     └─► loop: parse_initializer_element [Stage 97: designator detection]
  │                 ├─► '.' IDENTIFIER '='  → AST_DESIGNATED_INIT (member)
  │                 ├─► '[' const_expr ']' '=' → AST_DESIGNATED_INIT (array index) [eval_const_expr]
  │                 └─► parse_initializer   (recursive for nested braces)
  └─► scalar → parse_assignment_expression
```

---

## Key Design Points

- **No separate IR** — the AST is the only intermediate representation. Code
  generation is a single-pass recursive walk over the AST.
- **Recursive descent, hand-written** — no parser-generator or grammar tables.
- **Constant-expression evaluation is compile-time** — `eval_const_expr` is
  called at parse time for `case` labels, enum values, array designator indices,
  and (stage 100) file-scope integer initializers; the result is stored
  directly as `AST_INT_LITERAL` (parse-time constant folding).
- **`sizeof(type-name)` in constant expressions** (stage 100) — `eval_const_primary`
  calls `parse_type_name` and `type_size()` directly; no new AST nodes.
- **`sizeof(void *)` fixed** (stage 100) — `parse_primary` now delegates to
  `parse_type_name` for all type-start tokens including `TOKEN_VOID`; bare
  `sizeof(void)` is still rejected via a post-parse `t->kind == TYPE_VOID` check.
- **Block-scope static aggregates** (stage 101) — `codegen_statement` SC_STATIC
  arm now handles `TYPE_ARRAY`, `TYPE_STRUCT`, and `TYPE_UNION` by registering
  the variable with `is_static=1` and a `Lstatic_func_N` label. Four codegen
  sites (`codegen_expression` VAR_REF, `emit_array_index_addr`,
  `emit_member_addr`, `codegen_emit_local_statics`) use RIP-relative
  `[rel label]` addressing for static aggregates — parallel to file-scope globals.
- **Single-pass codegen** — labels are forward-declared using nasm's `jmp`-to-label
  mechanism; no backpatching infrastructure.
- **x86_64 Intel syntax** — all codegen emits Intel-syntax NASM. No AT&T syntax.
- **Self-hosting** — the compiler can compile itself. The bootstrap (C0→C1→C2)
  is exercised as part of each stage's self-host test via `./build.sh --mode=self-host`.

---

## Known Gaps (as of stage 101)

- **Self-hosting (stages 92–101):** the compiler compiles itself. C0
  (gcc-built) produces C1, and C1 produces C2; all three pass the full
  1552-test suite, confirming bootstrap stability. Stages 96 through 101 all
  passed self-hosting with no new issues. See `docs/self-compilation-report.md`.
- **Still parser-rejected (known gaps):** functions returning function pointers;
  pointer-to-array declarators (`(*p)[10]`); old-style (K&R) function
  definitions; chained designators (`.a.b`, `.arr[0]`); designated union init
  for non-first members; compound literals at file scope.
- **Designated initializers in block-scope static arrays** are not yet supported
  (the `codegen_emit_local_statics` path currently rejects `AST_DESIGNATED_INIT`
  children in static array initializers — plain sequential brace-lists only).
- **Multidimensional static arrays** and **static arrays of structs** are not
  yet supported (single-dimension integer/char element arrays only).
- **Block-scope `static` variable constant-expression initializers** (scalar
  statics) are currently unchecked — out of scope.
