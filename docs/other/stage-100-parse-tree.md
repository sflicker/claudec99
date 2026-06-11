
# Full Grammar Parse Tree — Stage 100

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
"error: compound literals at file scope are not yet supported".

---

**Stage 100 wires `eval_const_expr` into file-scope integer initializers and
extends `eval_const_primary` with `sizeof(type-name)`.** No new AST node types.
Three parser changes:

1. **`eval_const_primary` extended with `sizeof(type-name)`** (`src/parser.c`):
   the `TOKEN_SIZEOF` branch is added before the fall-through error. Logic:
   consume `sizeof`, require `TOKEN_LPAREN`, consume `(`, check if current token
   starts a type-name (type-start condition: `TOKEN_VOID`, `TOKEN_CONST`,
   `TOKEN_VOLATILE`, `TOKEN_BOOL`, `TOKEN_CHAR`, `TOKEN_SHORT`, `TOKEN_INT`,
   `TOKEN_LONG`, `TOKEN_SIGNED`, `TOKEN_UNSIGNED`, `TOKEN_STRUCT`, `TOKEN_UNION`,
   `TOKEN_ENUM`, or a typedef identifier), call `parse_type_name(parser)` →
   `Type *t`, check `t->kind == TYPE_VOID` (reject bare `sizeof(void)`) and
   incomplete array (`t->kind == TYPE_ARRAY && t->length == 0`), then
   `parser_expect(TOKEN_RPAREN)` and `return (long)type_size(t)`. This enables
   `sizeof(int) * 256`, `sizeof(struct Pair)`, `sizeof(void *)`, and
   `sizeof(enum Color)` in constant-expression contexts.
   - `TOKEN_ENUM` added (was absent from `parse_primary`'s sizeof arm — a
     pre-existing gap; `sizeof(enum Color)` is always `sizeof(int)`).
   - `TOKEN_VOID` added so `sizeof(void *)` works (bare `void` still rejected
     via the `t->kind == TYPE_VOID` check after `parse_type_name`).

2. **`parse_external_declaration` first-declarator path updated** (`src/parser.c`):
   the non-brace initializer arm is replaced with a branch on declaration type:
   - `decl->decl_type != TYPE_POINTER && != TYPE_STRUCT && != TYPE_UNION` →
     calls `eval_const_expr(parser, "file-scope initializer")`, formats the
     result into a 32-byte buffer, stores it via `lexer_store_bytes`, and
     creates an `AST_INT_LITERAL` node — identical to what codegen already
     handles for integer global initializers (no codegen changes needed).
   - Otherwise (pointer, struct, union) → keeps original `parse_assignment_expression`
     path with compound-literal detection and literal-only check.

3. **`parse_external_declaration` multi-declarator path updated** (`src/parser.c`):
   the comma-separated declarator list arm replaces `parse_primary` with the
   same branch using `k2` (the TypeKind already computed for the second
   declarator): `k2 != TYPE_POINTER` → `eval_const_expr`; pointer path retains
   `parse_primary` with literal check.

**`parse_primary` sizeof arm bug fix** (`src/parser.c`): the pre-existing early
rejection of `TOKEN_VOID` after `sizeof(` is removed. `TOKEN_VOID` is added to
the type-start condition, and a `t->kind == TYPE_VOID` check is added after
`parse_type_name` — matching the new pattern in `eval_const_primary`. This
makes `sizeof(void *)` work in general expression contexts (not just constant
ones), fixing a pre-existing gap.

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
- **Single-pass codegen** — labels are forward-declared using nasm's `jmp`-to-label
  mechanism; no backpatching infrastructure.
- **x86_64 Intel syntax** — all codegen emits Intel-syntax NASM. No AT&T syntax.
- **Self-hosting** — the compiler can compile itself. The bootstrap (C0→C1→C2)
  is exercised as part of each stage's self-host test via `./build.sh --mode=self-host`.

---

## Known Gaps (as of stage 100)

- **Self-hosting (stages 92–100):** the compiler compiles itself. C0
  (gcc-built) produces C1, and C1 produces C2; all three pass the full
  1544-test suite, confirming bootstrap stability. The original bootstrap
  (stages 91-01/92) surfaced and fixed thirteen real defects (struct-by-value
  ABI, the silent AST-truncation bug, six struct-by-value/`sizeof` codegen bugs,
  duplicate-`extern` emission, literal/switch-label capacity limits, missing
  `global` directives, and `strtol` stubs). Stage 94 added the repeatable
  `build.sh --mode=self-host` C0→C1→C2 cycle, and the 95-xx storage-refactor
  stages each re-verified self-hosting. Stages 96 through 100 all passed
  self-hosting with no new issues. See `docs/self-compilation-report.md`.
- **Still parser-rejected (known gaps):** functions returning function pointers;
  pointer-to-array declarators (`(*p)[10]`); old-style (K&R) function
  definitions; chained designators (`.a.b`, `.arr[0]`); designated union init
  for non-first members; compound literals at file scope. Block-scope static
  arrays remain unsupported in codegen. Block-scope `static` variable
  constant-expression initializers (currently unchecked — out of scope).
