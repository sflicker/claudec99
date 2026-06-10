# Stage 98 Kickoff — Compound Literals

## Summary

Add C99 compound literals — the `(type-name) { initializer-list }` expression
syntax (§6.5.2.5) that produces an unnamed temporary object of the specified
type, initializes it from the brace-list, and evaluates to an lvalue.

```c
/* Struct compound literal as a function argument */
draw_point((struct Point){ .x = 10, .y = 20 });

/* Array compound literal decaying to pointer */
memcpy(dst, (int[]){ 1, 2, 3, 4 }, 4 * sizeof(int));

/* Address-of a compound literal */
int *p = &(int){ 42 };

/* Compound literal with postfix access */
int y = (struct Point){ .x = 1, .y = 5 }.y;   /* 5 */
```

Compound literals are a core C99 feature that enables passing aggregate values
without naming a temporary variable and constructing structs/arrays in-line at
call sites. This is a **language feature stage** touching the parser, AST, and
code generator. No tokenizer changes. The self-host cycle must continue to pass.

---

## Tokenizer Changes

None. Compound literals use only existing tokens: `(`, `)`, `{`, `}`, `.`, `[`,
`]`, `=`.

---

## Parser Changes

**Task 2a: Extract `parse_postfix_suffixes()` helper** (`src/parser.c`)

The postfix-suffix loop in `parse_postfix()` currently handles `[expr]`, `.field`,
`->field`, `(args)`, `++`, `--` inline. Extract into a new static helper:

```c
static ASTNode *parse_postfix_suffixes(Parser *parser, ASTNode *base);
```

This enables compound literals to chain postfix operations. `parse_postfix()`
becomes:

```c
static ASTNode *parse_postfix(Parser *parser) {
    ASTNode *base = parse_primary(parser);
    return parse_postfix_suffixes(parser, base);
}
```

**Task 2b: Compound literal detection in `parse_cast()`** (`src/parser.c`)

In `parse_cast()`, after the type-name has been parsed and the closing `)` has
been consumed, check whether the next token is `TOKEN_LBRACE`:

```
if current == TOKEN_LBRACE:
    node = build_compound_literal(parser, type_info)
    return parse_postfix_suffixes(parser, node)
else:
    inner = parse_cast(parser)
    return AST_CAST(type_info, inner)
```

**Task 2c: `build_compound_literal()` helper** (`src/parser.c`)

A static helper that consumes `{ initializer-list }` and produces an
`AST_COMPOUND_LITERAL` node:

```c
static ASTNode *build_compound_literal(Parser *parser, /* type info */);
```

Pseudocode:

```
1. Consume TOKEN_LBRACE.

2. Determine the element type and parse the initializer list.

   a. If the type is a struct/union: parse a brace-list using
      parse_initializer_element. Set decl_type = TYPE_STRUCT/TYPE_UNION,
      full_type = the struct/union Type*.

   b. If the type is an array with explicit length N:
      parse a brace-list; length is N.
      Set decl_type = TYPE_ARRAY, full_type = type_array(elem, N).

   c. If the type is an array with omitted first dimension (has_size == 0):
      parse a brace-list, then infer the length from the highest [N]
      designator index or plain-element count.
      Build full_type = type_array(elem_type, inferred_length).

   d. If the type is a scalar (int, char, pointer, …):
      parse a single assignment_expression (or brace-enclosed single value).

3. Consume TOKEN_RBRACE (allow trailing comma before it).

4. Create AST_COMPOUND_LITERAL node:
       node->decl_type   = derived base kind
       node->full_type   = Type* (or NULL for plain scalars)
       node->byte_length = 0   /* set by pre-scan later */
       node->is_unsigned = ...
       ast_add_child(node, list)

5. Return node.
```

**Task 2d: Type-name update for `parse_type_name()`** (`src/parser.c`)

Allow `[]` (omitted first array dimension) in `parse_type_name()`, producing
`type_array(elem, 0)`. This is only valid in compound-literal context; other uses
remain errors.

**Task 2e: Error handling**

- **File-scope compound literals:** Detected in codegen (not parser).
  Emit `compile_error("error: compound literals at file scope are not yet supported\n")`.

- **Unsupported types (void, function, VLA):** Emit parse error:
  `compile_error("error: invalid type for compound literal\n")`.

---

## AST Changes

**Add `AST_COMPOUND_LITERAL`** to `ASTNodeType` enum (`include/ast.h`), after
`AST_DESIGNATED_INIT`:

```c
AST_COMPOUND_LITERAL,  /* stage 98: (type-name){ initializer-list } */
```

Node layout:

| Field         | Usage |
|---------------|-------|
| `decl_type`   | base type kind (TYPE_INT, TYPE_STRUCT, TYPE_ARRAY, TYPE_POINTER, …) |
| `full_type`   | `Type *` for struct/union/array/pointer; NULL for plain scalars |
| `byte_length` | stack offset relative to `rbp` (negative); assigned during pre-scan |
| `is_unsigned` | 1 if the base scalar type is unsigned |
| `value`       | NULL (unnamed) |
| `children[0]` | `AST_INITIALIZER_LIST` (or a plain expr for scalar) |

`child_count` is always 1.

The `byte_length` field has a **dual meaning**: at parse time it is 0; during
the pre-scan it is overwritten with the pre-assigned `rbp`-relative stack offset.
The object size is always recoverable from `full_type` or
`type_kind_bytes(decl_type)`.

**`print_ast` output** (for the print_ast test suite):

```
COMPOUND_LITERAL(struct Point)
  INITIALIZER_LIST
    DESIGNATED_INIT(.x)
      INT_LITERAL(1)
    DESIGNATED_INIT(.y)
      INT_LITERAL(2)
```

For an array:
```
COMPOUND_LITERAL(int[4])
  INITIALIZER_LIST
    INT_LITERAL(1)
    INT_LITERAL(2)
    INT_LITERAL(0)
    INT_LITERAL(0)
```

For a scalar:
```
COMPOUND_LITERAL(int)
  INT_LITERAL(42)
```

Add a `case AST_COMPOUND_LITERAL:` branch to `src/ast_pretty_printer.c` that
renders the type label and the initializer child, indented.

---

## Code Generation Changes

### Task 3: Pre-scan — extend frame-size computation

The function prologue codegen walks the function body AST before emitting
`sub rsp, N` to compute the total stack frame size. Extend it to walk expression
children and accumulate space for `AST_COMPOUND_LITERAL` nodes, assigning each
one a stack offset.

**Task 3a: Find the pre-scan function** (`src/codegen.c`)

Locate the function that walks a function body and accumulates `cg->local_offset`
before the prologue is emitted. If no separate function exists, introduce one.

**Task 3b: Add `scan_expr_compound_literals()` helper** (`src/codegen.c`)

A new static helper that recursively walks an expression subtree:

```c
static void scan_expr_compound_literals(CodeGen *cg, ASTNode *node);
```

For each node it visits:

- If `node->type == AST_COMPOUND_LITERAL`:
  1. Determine object size `sz`:
     - If `node->full_type != NULL`: `sz = node->full_type->size`
     - Else (scalar): `sz = type_kind_bytes(node->decl_type)`
  2. Determine alignment `align`:
     - If `node->full_type != NULL`: `align = node->full_type->alignment`
     - Else: `align = sz` (scalars are self-aligned)
  3. Align `cg->local_offset` up to `align`.
  4. Advance `cg->local_offset += sz`.
  5. Store the offset: `node->byte_length = cg->local_offset`.
  6. Recurse into `children[0]` (initializer list).

- For every other node type: recurse into all children.

**Task 3c: Call from the pre-scan pass**

The pre-scan must call `scan_expr_compound_literals()` for:
- Every expression statement child
- Every declaration's initializer expression
- Every condition expression (if/while/do-while/for/switch)
- Every for-update expression
- Every return expression
- Every function-call argument list

---

### Task 4: Codegen — struct compound literal

In `codegen_expr()`, add a `case AST_COMPOUND_LITERAL:` branch.

For **struct/union** types:

```
offset = node->byte_length        /* positive; address is rbp - offset */
list   = node->children[0]        /* AST_INITIALIZER_LIST */
st     = node->full_type          /* struct/union Type* */

/* 1. Zero-fill the struct slot */
for (b = 0; b < st->size; b++):
    emit "    mov byte [rbp - %d], 0\n", offset - b

/* 2. Apply initializers using emit_local_struct_init */
emit_local_struct_init(cg, st, offset, list)

/* 3. Leave address in rax */
emit "    lea rax, [rbp - %d]\n", offset
```

**File-scope guard**: If `cg->local_offset == 0` (or equivalent), emit:

```
compile_error("error: compound literals at file scope are not yet supported\n")
```

---

### Task 5: Codegen — array compound literal

For **array** types:

```
offset   = node->byte_length
list     = node->children[0]       /* AST_INITIALIZER_LIST */
arr_type = node->full_type         /* type_array(elem_type, length) */
length   = arr_type->length
elem_size = (size of array elements)
size      = arr_type->size         /* total bytes = length * elem_size */

/* Phase 1: zero-fill */
for (b = 0; b < size; b++):
    emit "    mov byte [rbp - %d], 0\n", offset - b

/* Phase 2: apply initializers with cursor (same as local array init) */
cur = 0
for (j = 0; j < list->child_count; j++):
    child = list->children[j]
    if child->type == AST_DESIGNATED_INIT && child->value == NULL:
        idx = child->byte_length
        if idx < 0 || idx >= length:
            compile_error("error: array designator index %d out of bounds\n", idx)
        cur = idx
        elem = child->children[0]
    else if child->type == AST_DESIGNATED_INIT:
        compile_error("error: member designator in array initializer\n")
    else:
        elem = child
    if cur >= length:
        compile_error("error: too many initializers in compound literal\n")
    elem_offset = offset - cur * elem_size
    /* ... emit elem at elem_offset ... */
    cur++

/* Leave address of first element in rax */
emit "    lea rax, [rbp - %d]\n", offset
```

---

### Task 6: Codegen — scalar compound literal

For **scalar** types (int, char, pointer, etc.):

```
offset = node->byte_length
sz     = type_kind_bytes(node->decl_type)   /* 1, 2, 4, or 8 */
init   = node->children[0]                   /* the initializer expression */

/* 1. Evaluate the initializer into rax */
codegen_expr(cg, init)

/* 2. Store the value at the compound literal's stack slot */
emit store of the appropriate width to [rbp - offset]

/* 3. Leave the value in rax */
/* The value is already in rax; the store is for lvalue semantics in case
 * of `&(int){5}`, which must have a real address. */
```

---

## Test Plan

**Valid tests (12):**
- Struct compound literal as function argument
- Struct compound literal assigned to a variable
- Array compound literal (explicit size) as pointer argument
- Array compound literal with omitted size
- Array compound literal with index designators
- Postfix member access on struct compound literal
- Postfix subscript on array compound literal
- Address-of a compound literal (scalar)
- Scalar compound literal
- Struct compound literal with zero-fill (partial init)
- Two compound literals as arguments (combined)
- Char array compound literal

**Invalid tests (4):**
- File-scope compound literal (not yet supported)
- Compound literal of void type
- Too many initializers in compound literal
- Array index out of bounds in compound literal

**`print_ast` tests (3):**
- Struct compound literal with member designators
- Array compound literal (explicit size)
- Scalar compound literal

**Integration test (1):**
- Multi-file test with shapes.c + main.c using compound literals

---

## Out of Scope

- **File-scope compound literals** (static storage duration). Emit
  `compile_error("error: compound literals at file scope are not yet supported\n")`.
- **Union compound literals with non-first member designator**. Emit
  `compile_error("error: union compound literal with non-first member designator not yet supported\n")`.
- **VLA compound literals** (`(int[n]){…}`). Not supported.
- **Nested compound literals in designator expressions** — already prevented by
  constant-expression restriction.
- **Compound literals as assignment targets** (`(int){ 5 } = 6`). Undefined behavior.
- **`print_tokens` updates** — no new tokens introduced.
- **Multidimensional arrays with omitted inner dimensions** — only the first
  (outermost) dimension may be omitted.

---

## Implementation Order

1. `include/ast.h` — add `AST_COMPOUND_LITERAL` enum value
2. `src/ast_pretty_printer.c` — add `case AST_COMPOUND_LITERAL:`
3. `src/parser.c`:
   a. Extract `parse_postfix_suffixes()` from `parse_postfix()`
   b. Update `parse_type_name()` to allow `[]` (omitted dimension)
   c. Add compound-literal detection in `parse_cast()`
   d. Add `build_compound_literal()` helper
4. `src/codegen.c`:
   a. Add `scan_expr_compound_literals()` helper
   b. Extend the frame-size pre-scan to call it
   c. Add `case AST_COMPOUND_LITERAL:` to `codegen_expr()`:
      - struct path → Task 4
      - array path → Task 5
      - scalar path → Task 6
5. `src/version.c` — bump `VERSION_STAGE` to `"00980000"`
6. Tests — add valid (12), invalid (4), `print_ast` (3), and integration (1)
7. Documentation: README, grammar.md, supplemental snapshots, self-host report

---

## Key Decisions

- **Frame-size pre-scan architecture:** Separate pre-scan function or extension
  required to assign stack offsets before any code is emitted.

- **Fixed-size array constants:** The recursive descent in `scan_expr_compound_literals()`
  uses no VLAs; children are iterated using existing patterns.

- **`byte_length` dual semantics:** Parse time = 0; post-scan = rbp-relative offset
  (positive integer; real address is `rbp - byte_length`).

- **Array size inference:** For `(int[])`, walk the initializer list to find the
  maximum index or element count; build `type_array(elem, inferred_length)`.

- **Postfix chaining:** Compound literals must participate in the postfix-suffix
  loop so expressions like `(struct Point){ .x=1 }.x` work correctly.

---

## Ambiguities Resolved

- **Omitted-size arrays:** `(int[])` with omitted size produces `type_array(int, 0)`
  at parse time; `build_compound_literal()` infers the length from the initializer
  list and sets `full_type = type_array(int, inferred_length)`.

- **File-scope detection:** Codegen check: if compound literal is encountered with
  `cg->local_offset == 0` or equivalent, emit error.

- **Union non-first-member designators:** Emit codegen error; parser accepts all
  union compound literals.

- **Too many initializers:** Detected during codegen at element-emission time.

- **Address-of compound literal:** Lvalue semantics are automatic: the
  compound-literal node is addressable, and `&` codegen produces `lea rax, [rbp - offset]`.

---

## Bootstrap Notes

- **VLA restriction:** The `scan_expr_compound_literals()` helper uses no VLAs.

- **`*(T **)vec_get(...)` split:** If any new code uses this pattern, split into
  two statements as in prior stages.

- **Self-host verification:** Run `./build.sh --mode=self-host` before closing.
  The compiler's own source does not use compound literals today; C0→C1→C2 is
  expected to pass without new bootstrap workarounds.
