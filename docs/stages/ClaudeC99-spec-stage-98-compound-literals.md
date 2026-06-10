# ClaudeC99 Stage 98 — Compound Literals

## Goal

Add support for **C99 compound literals** — the `(type-name) { initializer-list }`
expression syntax (§6.5.2.5) that produces an unnamed temporary object of the
specified type, initializes it from the brace-list, and evaluates to an lvalue.

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

Compound literals are a core C99 feature that enables:
- Passing aggregate values without naming a temporary variable
- In-line construction of struct/array values at call sites
- Idiomatic zero- or partially-initialized temporaries

---

## Background — current context

The compiler already has all the required supporting machinery:

- **Designated initializers** (stage 97): `AST_DESIGNATED_INIT`,
  `parse_initializer_element`, cursor-based init in all four init sites.
- **Local struct init** (`emit_local_struct_init`): initializes a struct on the
  stack from an `AST_INITIALIZER_LIST`, including designated members.
- **Local array init** (in `codegen_statement`): two-phase (zero-fill +
  cursor-driven); handles `AST_DESIGNATED_INIT` for array indices.
- **`parse_type_name`** (`src/parser.c`): parses a type-name including struct,
  union, typedef, array, and pointer specifiers; returns a `ParsedDeclarator`.
- **`parse_cast`** (`src/parser.c`): already performs the `(type-name)`
  two-token lookahead needed for type-name detection.
- **Postfix-suffix loop** in `parse_postfix`: handles `[expr]`, `.field`,
  `->field`, `(args)`, `++`, `--` on any base expression.
- **Frame-size pre-scan**: the function-prologue emitter walks the AST to
  accumulate total local variable space before emitting `sub rsp, N`.

This stage connects them: compound literals are parsed in `parse_cast`, use the
local-init machinery for initialization, and are integrated into the frame-size
pre-scan so their stack slots are reserved before the prologue is emitted.

---

## Grammar update

In C99 compound literals are technically `<postfix_expression>` primaries, but
the disambiguation `(type-name) {` vs `(expr)` is most naturally resolved where
the type-name lookahead already lives — in `parse_cast`.

Add the compound-literal alternative to `<cast_expression>`:

```ebnf
<cast_expression> ::= "(" <type_name> ")" "{" <initializer_list> [ "," ] "}"
                    | "(" <type_name> ")" <cast_expression>
                    | <unary_expression>
```

Because compound literals must participate in the postfix-suffix loop (`.field`,
`[idx]`, etc.), a helper `parse_postfix_suffixes` is extracted from `parse_postfix`
and called on the compound-literal node before `parse_cast` returns (see Task 2).

The `<type_name>` in a compound literal may include an omitted first array
dimension: `(int[])`, `(char[])`.  This is only valid in the compound-literal
context; `sizeof(int[])` remains an error.

---

## Task 1 — New AST node: `AST_COMPOUND_LITERAL`

Add to the `ASTNodeType` enum in `include/ast.h` (after `AST_DESIGNATED_INIT`):

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
| `children[0]` | `AST_INITIALIZER_LIST` (or a plain `parse_assignment_expression` for scalar) |

`child_count` is always 1.

The `byte_length` field has a **dual meaning** for this node type: at parse
time it is 0; during the pre-scan (Task 3) it is overwritten with the
pre-assigned `rbp`-relative stack offset (a positive integer; the actual address
is `rbp - byte_length`).  The object size is always recoverable from `full_type`
or `type_kind_bytes(decl_type)`.

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

The type label in parentheses should describe the type concisely.  A helper
`type_to_string` (new, or a minimal inline rendering) is sufficient: `"int"`,
`"unsigned int"`, `"char"`, `"struct Tag"`, `"int[N]"`, `"int *"`, etc.
Exact formatting is not tested beyond what the `print_ast` test files assert.

Add a `case AST_COMPOUND_LITERAL:` branch to `src/ast_pretty_printer.c`.

---

## Task 2 — Parser: detect and parse compound literals

### 2a — Extract `parse_postfix_suffixes` helper

The postfix-suffix loop in `parse_postfix` currently lives inline.  Extract it
into a new static helper:

```c
/* Apply any trailing postfix suffixes ([expr], .field, ->field, (args),
 * ++, --) to `base` and return the final expression node. */
static ASTNode *parse_postfix_suffixes(Parser *parser, ASTNode *base);
```

`parse_postfix` becomes:

```c
static ASTNode *parse_postfix(Parser *parser) {
    ASTNode *base = parse_primary(parser);
    return parse_postfix_suffixes(parser, base);
}
```

### 2b — Compound literal detection in `parse_cast`

In `parse_cast`, after the type-name has been parsed and the closing `)` has been
consumed, test whether the next token is `TOKEN_LBRACE`:

```
if current == TOKEN_LBRACE:
    /* Compound literal */
    node = build_compound_literal(parser, type_info)
    return parse_postfix_suffixes(parser, node)
else:
    /* Cast: apply to the inner expression */
    inner = parse_cast(parser)
    return AST_CAST(type_info, inner)
```

Calling `parse_postfix_suffixes` on the compound literal node enables chains
like `(struct Point){ .x=1 }.x` and `(int[]){ 1, 2, 3 }[1]`.

### 2c — `build_compound_literal` helper

A static helper that consumes the `{` initializer-list `}` and produces an
`AST_COMPOUND_LITERAL` node:

```c
static ASTNode *build_compound_literal(Parser *parser, /* type info from parse_type_name */);
```

Pseudocode:

```
1.  Consume TOKEN_LBRACE.

2.  Determine the element type and parse the initializer list.

    a.  If the type is a struct/union: parse a brace-list using
        parse_initializer_element (exactly as parse_initializer does for
        declarations).  Set decl_type = TYPE_STRUCT/TYPE_UNION,
        full_type = the struct/union Type*.

    b.  If the type is an array with an explicit length N:
        parse a brace-list; length is N.
        Set decl_type = TYPE_ARRAY, full_type = type_array(elem, N).

    c.  If the type is an array with an omitted first dimension (has_size == 0):
        parse a brace-list first, then infer the length:
            length = infer_compound_literal_array_length(list)
        where infer_compound_literal_array_length walks list children and
        returns:
            max(highest [N] designator index + 1, plain_element_count)
        Build full_type = type_array(elem_type, length).

    d.  If the type is a scalar (int, char, pointer, …):
        parse a single assignment_expression (or a brace-enclosed single
        value; accept both).

3.  Consume TOKEN_RBRACE (allow trailing comma before it, matching
    parse_initializer).

4.  Create AST_COMPOUND_LITERAL node:
        node->decl_type   = derived base kind
        node->full_type   = Type* (or NULL for plain scalars)
        node->byte_length = 0   /* set by pre-scan later */
        node->is_unsigned = ...
        ast_add_child(node, list)

5.  Return node.
```

### 2d — Error: file-scope compound literals

If a compound literal appears outside a function body (detected during
codegen — see Task 4) emit:

```
compile_error("error: compound literals at file scope are not yet supported\n")
```

The parser does not need to detect this; codegen enforces it.

### 2e — Error: unsupported types

Compound literals of `void`, function, or VLA types emit a compile_error in
the parser:

```
compile_error("error: invalid type for compound literal\n")
```

These cases can be detected immediately after `parse_type_name` when building
the compound literal.

---

## Task 3 — Pre-scan: extend frame-size computation

The function-prologue codegen walks the function body AST before emitting
`sub rsp, N` to compute the total stack frame size.  This walk currently handles
`AST_DECLARATION` nodes but does not descend into expression trees.

Extend it to also walk expression children and accumulate space for any
`AST_COMPOUND_LITERAL` nodes encountered, assigning each one a stack offset.

### 3a — Where the pre-scan lives

Find the function (probably called something like `compute_local_frame_size`,
`collect_locals`, or similar) in `src/codegen.c` that walks a function body and
accumulates the running `cg->local_offset` (or returns a total).  It is called
once before the prologue is emitted.

If no such separate function exists and the locals are accumulated in a single
declaration-processing walk, introduce one so that compound literal offsets can
be pre-assigned before any code is emitted.

### 3b — Recursive expression descent

Add a new static helper:

```c
/* Walk an expression subtree and reserve stack space for any
 * AST_COMPOUND_LITERAL nodes, assigning their stack offsets. */
static void scan_expr_compound_literals(CodeGen *cg, ASTNode *node);
```

For each node it visits:
- If `node->type == AST_COMPOUND_LITERAL`:
  1. Determine object size `sz`:
     - If `node->full_type != NULL`: `sz = node->full_type->size`
     - Else (scalar): `sz = type_kind_bytes(node->decl_type)`
  2. Determine alignment `align` (same rules as local variable alignment):
     - If `node->full_type != NULL`: `align = node->full_type->alignment`
     - Else: `align = sz`  (scalars are self-aligned)
  3. Align `cg->local_offset` up to `align`.
  4. Advance `cg->local_offset += sz`.
  5. Store the offset: `node->byte_length = cg->local_offset`.
  6. **Recurse into children[0]** (the initializer list) to catch nested
     compound literals.
- For every other node type: recurse into all children.

### 3c — Call from the pre-scan pass

The pre-scan must call `scan_expr_compound_literals` for:
- Every expression statement child
- Every declaration's initializer expression
- Every condition expression (if/while/do-while/for/switch)
- Every for-update expression
- Every return expression
- Every function-call argument list

The simplest approach: make the pre-scan helper recursively descend all AST
children (both statement and expression nodes), calling the compound-literal
accumulator for each `AST_COMPOUND_LITERAL` encountered.  Statement nodes that
contain further statement blocks (if/while/for/switch bodies) are already
recursed by the existing scan.

---

## Task 4 — Codegen: struct compound literal

In `codegen_expr` (or `codegen_emit_expr`, whichever handles expression nodes),
add a `case AST_COMPOUND_LITERAL:` branch.

**Preconditions**: `node->byte_length` holds the pre-assigned `rbp`-relative
offset (set by Task 3).  The type is TYPE_STRUCT or TYPE_UNION.

```
offset = node->byte_length        /* positive; address is rbp - offset */
list   = node->children[0]        /* AST_INITIALIZER_LIST */
st     = node->full_type          /* struct/union Type* */

/* 1. Zero-fill the struct slot. */
for (b = 0; b < st->size; b++):
    emit "    mov byte [rbp - %d], 0\n", offset - b

/* 2. Apply initializers using emit_local_struct_init. */
emit_local_struct_init(cg, st, offset, list)

/* 3. Leave address in rax. */
emit "    lea rax, [rbp - %d]\n", offset
```

The struct is now initialized at its stack slot and `rax` holds a pointer to it.

**Result type**: the compound literal expression has type `struct Tag *` in
codegen terms (its address is in rax).  When the compound literal is used as a
value argument to a struct-by-value call, the caller-side struct-argument
machinery already handles a pointer-to-struct source correctly (same as
`emit_struct_copy` receiving a local variable's address).

**File-scope guard**: if `cg->local_offset == 0` and no function is being
compiled (or use whatever indicator the codegen uses to detect file-scope
context), emit:

```
compile_error("error: compound literals at file scope are not yet supported\n")
```

---

## Task 5 — Codegen: array compound literal

Same `case AST_COMPOUND_LITERAL:` branch, TYPE_ARRAY path.

```
offset   = node->byte_length
list     = node->children[0]       /* AST_INITIALIZER_LIST */
arr_type = node->full_type         /* type_array(elem_type, length) */
length   = arr_type->length
elem_type = arr_type->elem         /* or arr_type->fields[0] for nested arrays */
elem_size = elem_type->size  (or type_kind_bytes for scalar elem)
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
    /* ... existing dispatch for scalar elem ... */
    cur++

/* Leave address of first element in rax (array decays to pointer) */
emit "    lea rax, [rbp - %d]\n", offset
```

**Omitted-size arrays at codegen**: `node->full_type->length` already holds the
inferred length (set during parsing by `infer_compound_literal_array_length`).
No special handling needed here.

---

## Task 6 — Codegen: scalar compound literal

Scalar types: `int`, `char`, `short`, `long`, `long long`, unsigned variants,
`_Bool`, pointer types.

```
offset = node->byte_length
sz     = type_kind_bytes(node->decl_type)   /* 1, 2, 4, or 8 */
init   = node->children[0]                   /* the initializer expression */

/* 1. Evaluate the initializer into rax. */
codegen_expr(cg, init)

/* 2. Store the value at the compound literal's stack slot. */
emit store of the appropriate width to [rbp - offset]

/* 3. Leave the value in rax (scalar compound literal evaluates to its value). */
/* (rax already holds the value from step 1; the store is for lvalue semantics
 *  only — `&(int){5}` must have a real address.) */
```

For pointer scalar types (`node->decl_type == TYPE_POINTER`), treat as 8-byte
scalar (same as pointer locals).

Note: scalar compound literals are technically lvalues in C99, but they are
almost always used as rvalues.  Since the value is left in rax after step 1,
common uses (passing as argument, comparing, etc.) work without a reload.  The
`&(int){5}` case works because the value was stored in step 2; address-of on the
compound literal node produces `lea rax, [rbp - offset]` through the normal
`&` codegen path (which checks that the operand is an addressable lvalue —
`AST_COMPOUND_LITERAL` is addressable).

---

## Task 7 — `print_ast` support

Add `case AST_COMPOUND_LITERAL:` to `src/ast_pretty_printer.c`.  Print a
descriptive label followed by the initializer child, indented:

```c
case AST_COMPOUND_LITERAL:
    /* Render a short type description. */
    if (node->full_type && node->full_type->kind == TYPE_STRUCT)
        printf("COMPOUND_LITERAL(struct %s)\n", node->full_type->tag ? node->full_type->tag : "<anon>");
    else if (node->full_type && node->full_type->kind == TYPE_ARRAY)
        printf("COMPOUND_LITERAL(%s[%d])\n", elem_kind_name, node->full_type->length);
    else
        printf("COMPOUND_LITERAL(%s)\n", decl_type_name(node->decl_type));
    /* Print the initializer child. */
    print_ast_node(node->children[0], depth + 1);
    break;
```

Exact label format is defined by the `print_ast` test expected files — adjust
to match whatever the tests assert.

---

## Out of scope — do NOT do these in this stage

- **File-scope compound literals** (static storage duration, §6.5.2.5p3).
  If encountered at file scope (in a global initializer or global declaration
  context), emit `compile_error("error: compound literals at file scope are not
  yet supported\n")`.
- **Compound literals of union type selecting a non-first member**.  `(union U){
  .b = 5 }` where `.b` is not the first field — the union codegen path currently
  only initializes the first field.  A member-designated union compound literal
  where the designator names a non-first field should emit `compile_error("error:
  union compound literal with non-first member designator not yet supported\n")`.
  A union compound literal with no designator or with a designator naming the
  first field is in scope and should work.
- **VLA compound literals** (`(int[n]){…}`).  Not supported.
- **Nested compound literals in a designator's index expression** — already
  prevented by the existing restriction that designator indices must be constant
  expressions (`eval_case_const_expr`).
- **Compound literals as assignment targets** (`(int){ 5 } = 6`).  While
  technically lvalues in C99, assigning to a compound literal is undefined
  behavior in practice.  The existing codegen does not need to handle this case.
- **`print_tokens` test updates** — no new tokens are introduced; the lexer is
  unchanged.
- **Multidimensional array compound literals with omitted inner dimensions** —
  only the first (outermost) dimension may be omitted; inner dimensions must be
  explicit.  Enforced by the existing `parse_type_name` logic.

---

## Bootstrap caveats

The compiler must continue to self-host (C0→C1→C2).  Three patterns to watch:

1. **VLAs** — All new local arrays in the pre-scan helper and compound-literal
   codegen functions must use compile-time constants, not VLAs.  The pre-scan
   helper recursively walks the tree using the existing `for`-over-children
   pattern — no fixed-size temp arrays are needed there.

2. **Const-expression initializers** — If any new function contains an
   initializer that is itself a compound literal in the compiler's own source,
   that will need this stage to compile.  Check `src/*.c` for any
   `(T){ … }` patterns and either avoid them or verify they are already absent.

3. **`*(T **)vec_get(...)` split** — as in prior stages, if any new `Vec` access
   uses a dereference-cast on the result of `vec_get`, split it into two
   statements.

Run `./build.sh --mode=self-host` before declaring the stage complete.

---

## Tests

### Valid tests

**Struct compound literal as function argument:**
```c
struct Point { int x; int y; };
int sum_point(struct Point p) { return p.x + p.y; }
int main() {
    return sum_point((struct Point){ .x = 3, .y = 4 });   /* expect 7 */
}
```

**Struct compound literal assigned to a variable:**
```c
struct Point { int x; int y; };
int main() {
    struct Point p = (struct Point){ .x = 10, .y = 20 };
    return p.x + p.y;   /* expect 30 */
}
```

**Array compound literal (explicit size) as pointer argument:**
```c
int sum_arr(int *a, int n) {
    int s = 0;
    for (int i = 0; i < n; i++) s += a[i];
    return s;
}
int main() {
    return sum_arr((int[4]){ 1, 2, 3, 4 }, 4);   /* expect 10 */
}
```

**Array compound literal with omitted size:**
```c
int sum_arr(int *a, int n) {
    int s = 0;
    for (int i = 0; i < n; i++) s += a[i];
    return s;
}
int main() {
    return sum_arr((int[]){ 5, 6, 7 }, 3);   /* expect 18 */
}
```

**Array compound literal with index designators:**
```c
int main() {
    int *a = (int[5]){ [2] = 10, [4] = 20 };
    return a[0] + a[2] + a[4];   /* expect 30 */
}
```

**Postfix member access on struct compound literal:**
```c
struct Point { int x; int y; };
int main() {
    int y = (struct Point){ .x = 1, .y = 99 }.y;
    return y;   /* expect 99 */
}
```

**Postfix subscript on array compound literal:**
```c
int main() {
    return (int[]){ 10, 20, 30 }[2];   /* expect 30 */
}
```

**Address-of a compound literal (scalar):**
```c
int main() {
    int *p = &(int){ 7 };
    *p = 8;
    return *p;   /* expect 8 */
}
```

**Scalar compound literal:**
```c
int main() {
    int x = (int){ 42 };
    return x;   /* expect 42 */
}
```

**Struct compound literal with zero-fill (partial init):**
```c
struct Three { int a; int b; int c; };
int sum3(struct Three t) { return t.a + t.b + t.c; }
int main() {
    return sum3((struct Three){ .b = 5 });   /* expect 5 */
}
```

**Compound literal in conditional expression:**
```c
struct Point { int x; int y; };
int dot(struct Point p, struct Point q) { return p.x * q.x + p.y * q.y; }
int main() {
    int result = dot(
        (struct Point){ .x = 2, .y = 3 },
        (struct Point){ .x = 4, .y = 5 }
    );
    return result;   /* 2*4 + 3*5 = 23 */
}
```

**Char array compound literal:**
```c
int strlen_cl(char *s) {
    int n = 0;
    while (s[n]) n++;
    return n;
}
int main() {
    return strlen_cl((char[]){ 'h', 'i', '\0' });   /* expect 2 */
}
```

### Invalid tests

**File-scope compound literal (not yet supported):**
```c
struct Point { int x; int y; };
struct Point *g = &(struct Point){ .x = 1 };
int main() { return 0; }
/* error: compound literals at file scope are not yet supported */
```

**Compound literal of void type:**
```c
int main() {
    (void){ };
    return 0;
}
/* error: invalid type for compound literal */
```

**Too many initializers:**
```c
int main() {
    return (int[2]){ 1, 2, 3 }[0];
    /* error: too many initializers in compound literal */
}
```

**Array index out of bounds in compound literal:**
```c
int main() {
    return (int[3]){ [5] = 1 }[0];
    /* error: array designator index 5 out of bounds */
}
```

### `print_ast` tests

One test each for:
- A struct compound literal with member designators
- An array compound literal (explicit size)
- A scalar compound literal

Each test verifies the `COMPOUND_LITERAL(...)` header line and the indented
initializer list structure.

### Integration test

One integration test (`test_compound_literal_multifile/`) with two source files:
- `shapes.c` — defines `struct Rect { int w; int h; };` and a function
  `rect_area(struct Rect r)` that returns `r.w * r.h`
- `main.c` — calls `rect_area` passing compound literals; also exercises an
  array compound literal as a pointer argument to a helper

The test verifies the final exit code.

---

## Implementation order

1. `include/ast.h` — add `AST_COMPOUND_LITERAL` after `AST_DESIGNATED_INIT`.
2. `src/ast_pretty_printer.c` — add `case AST_COMPOUND_LITERAL:` (needed early
   for `print_ast` tests).
3. `src/parser.c`:
   a. Extract `parse_postfix_suffixes` helper from `parse_postfix`.
   b. Add compound-literal detection in `parse_cast`.
   c. Add `build_compound_literal` helper.
4. `src/codegen.c`:
   a. Add `scan_expr_compound_literals` helper.
   b. Extend the frame-size pre-scan to call it.
   c. Add `case AST_COMPOUND_LITERAL:` to `codegen_expr`:
      struct path → Task 4; array path → Task 5; scalar path → Task 6.
5. `src/version.c` — bump `VERSION_STAGE` to `"00980000"`.
6. Tests — add valid, invalid, `print_ast`, and integration tests.
7. Documentation (see below).

---

## Docs (at stage close, after all tests pass)

- **`README.md`** — add "compound literals" to the feature summary; refresh
  aggregate test totals.
- **`docs/grammar.md`** — update `<cast_expression>` to include the compound
  literal alternative.
- Run the **`update-supplemental-docs`** skill to refresh the feature checklist,
  parse-tree document, and status/features snapshots through stage 98.
- **`docs/self-compilation-report.md`** — record the stage-98 self-host run.
- Update **`src/version.c`** (`VERSION_STAGE "00980000"`).
