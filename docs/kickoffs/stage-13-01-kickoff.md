# Stage-13-01 Kickoff: Array Types

## Spec Summary
Stage 13-01 introduces array declarations as a new local-variable
kind. The type system gets a new `TYPE_ARRAY` kind that records its
element type, length, and total size. Declarations of the form
`<type> <id> "[" <integer_literal> "]" ";"` are accepted for any
existing scalar element type and for pointer-to-scalar element types
(e.g. `int *ptrs[2];`). The size must be a positive integer literal;
`a[0]`, `a[-1]`, and `a[x]` are rejected. Codegen allocates
`sizeof(element) * length` bytes on the stack and stores the
variable's rbp-relative base offset in the locals table the same way
scalars are tracked, just with a larger size. Arrays are not
assignable. Indexing, decay, pointer arithmetic, multi-dimensional
arrays, array initializers, and globals are all out of scope.

## Spec issues / clarifications
1. **Grammar vs. Out of Scope:** The grammar permits `[ "=" <expression> ]`
   after the optional array brackets, but "Out of Scope" lists
   "Array initializers". Implementation will reject `int a[3] = …;`
   in the parser with a clear error.
2. **`int a[-1]` rejection mechanism:** `-1` lexes as two tokens
   (`-`, `1`). Restricting the size production to a single
   `<integer_literal>` causes `parser_expect(TOKEN_INT_LITERAL)` to
   fail on `-`, which yields the desired rejection.
3. **"Same as scalars, but larger size":** The existing locals table
   already stores `(name, offset, size, kind, full_type)`; arrays
   reuse the same record with `kind = TYPE_ARRAY` and the array's
   total byte size.
4. **Array variable use in expressions:** Spec only says "Arrays are
   NOT assignable" and lists indexing/decay as out of scope. To avoid
   producing nonsense code, `AST_VAR_REF` of an array local will
   error explicitly. The spec test cases never read array variables.
5. **Alignment:** Existing add-var aligns to `size` (works for
   `size ∈ {1,2,4,8}`). For an `int[3]` (size 12) we want element
   alignment (4). Threading an explicit `align` argument keeps
   scalars unchanged and gives arrays correct element-aligned slots.

## What changes from Stage 12-06
- Tokenizer: add `TOKEN_LBRACKET` / `TOKEN_RBRACKET` and lex `[` / `]`.
- Type system: add `TYPE_ARRAY`, `type_array(elem, length)`, and
  extend `type_kind_name` / `type_size` / `type_alignment`.
- AST: no new node type; `AST_DECLARATION` already carries
  `decl_type` and `full_type`. For an array, `decl_type` becomes
  `TYPE_ARRAY` and `full_type` carries the array Type.
- Parser: in declarations, after the identifier, optionally consume
  `"[" <integer_literal> "]"` and validate `length > 0`. If `=`
  follows the brackets, error (array initializers out of scope).
- Codegen:
  - `type_kind_bytes` returns `total_size` for `TYPE_ARRAY`.
  - `codegen_add_var` accepts an `align` parameter (scalars pass
    `size`; arrays pass element size).
  - `compute_decl_bytes` accounts for the actual array byte size
    plus a conservative alignment slack.
  - `AST_DECLARATION` for `TYPE_ARRAY` allocates the slot; no init
    path is taken.
  - `AST_VAR_REF` of an array local errors.
  - `AST_ASSIGNMENT` errors when the LHS local is an array.
- Pretty printer: render `VariableDeclaration: <element-type> <name>[<length>]`.
- Grammar doc: update `<declaration>` production.

## Planned Changes
1. **Tokenizer** (`include/token.h`, `src/lexer.c`)
   - Add `TOKEN_LBRACKET`, `TOKEN_RBRACKET`.
   - Lex `[` → `TOKEN_LBRACKET`, `]` → `TOKEN_RBRACKET`.
2. **Type system** (`include/type.h`, `src/type.c`)
   - Add `TYPE_ARRAY` to the `TypeKind` enum.
   - Extend `Type` with `int length` (used only when `kind == TYPE_ARRAY`).
   - Add `Type *type_array(Type *element_type, int length)` (heap-allocated; no freeing, consistent with `type_pointer`).
   - `type_kind_name`: map `TYPE_ARRAY` → `"array"`.
   - `type_size`: returns the recorded size (which already encodes total bytes).
   - `type_is_integer`: returns 0 for arrays.
3. **AST** — no new node type. `AST_DECLARATION` uses
   `decl_type = TYPE_ARRAY` and `full_type = <array Type>`.
4. **Parser** (`src/parser.c`)
   - In `parse_statement` declaration branch, after the identifier
     name, look for `TOKEN_LBRACKET`. If present:
     - consume `[`, expect `INT_LITERAL` (positive),
       expect `]`.
     - reject `length <= 0` with a clear error.
     - element type = current `full_type` (the integer base or
       pointer chain).
     - `decl->decl_type = TYPE_ARRAY`; `decl->full_type = type_array(...)`.
     - if next token is `=`, error: array initializers not
       supported.
   - No change to function parameters or function return types
     (arrays as parameters / return types are out of scope this
     stage and never appear in spec examples).
5. **Code generator** (`src/codegen.c`)
   - `type_kind_bytes(TYPE_ARRAY)`: caller must use the Type to get
     the size; helper returns 0 to make the dependency on
     `full_type` explicit. Actual sizing is done from `full_type->size`
     at the declaration site.
   - `codegen_add_var`: add `int align` parameter.
   - Update existing call sites to pass `align = size`.
   - Update `compute_decl_bytes` to account for the actual array
     byte size when `decl_type == TYPE_ARRAY`.
   - `AST_DECLARATION` for `TYPE_ARRAY`:
     - size = `node->full_type->size`
     - align = element type's `size` (natural alignment)
     - allocate, no initializer evaluated (parser rejected init).
   - `AST_VAR_REF` of an array local: error
     ("error: cannot use array variable '<name>' as a value").
   - `AST_ASSIGNMENT` to an array local: error
     ("error: arrays are not assignable").
6. **Pretty printer** (`src/ast_pretty_printer.c`)
   - `AST_DECLARATION` with `decl_type == TYPE_ARRAY`: render the
     element type then `<name>[<length>]`.
7. **Grammar doc** (`docs/grammar.md`)
   - Update `<declaration>` to:
     `<type> <identifier> [ "[" <integer_literal> "]" ] [ "=" <expression> ] ";"`
8. **Tests** (`test/valid/`, `test/invalid/`, `test/print_ast/`)
   - Valid (returns 0):
     - `test_array_int_3__0.c` — basic `int a[3];`
     - `test_array_mixed_locals__0.c` — int x; int a[3]; int y;
     - `test_array_char_long__0.c` — char a[4]; long b[2];
     - `test_array_short__0.c` — short s[4];
     - `test_array_of_pointers__0.c` — int *ptrs[2];
   - Invalid:
     - `test_invalid_32__array_size_must_be_positive.c` (`int a[0];`)
     - `test_invalid_33__expected_token.c` (`int a[-1];` — falls
       through to `expected token type` from
       `parser_expect(TOKEN_INT_LITERAL)`)
     - `test_invalid_34__array_size_must_be_an_integer_literal.c`
       (`int a[x];`)
     - `test_invalid_35__arrays_are_not_assignable.c`
       (`int a[3]; int b[3]; a = b;`)
     - `test_invalid_36__array_initializers_not_supported.c`
       (`int a[3] = 5;`)
   - Print-AST:
     - `test_stage_13_01_arrays.c` / `.expected` — covers a few of
       the declaration shapes.
9. **Commit** at the end with a concise message.
