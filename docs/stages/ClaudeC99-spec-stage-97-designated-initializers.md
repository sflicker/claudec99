# ClaudeC99 Stage 97 — Designated Initializers

## Goal

Add support for **C99 designated initializers** — the `.member = value` and
`[index] = value` syntactic forms that let a brace initializer name the target
field or element instead of relying on positional order.

```c
struct Point { int x, y; };
struct Point p = { .y = 10, .x = 3 };   /* member designators */

int a[6] = { [2] = 5, [5] = 9 };        /* array index designators */

struct Point pts[3] = {                  /* mixed designated + plain */
    [1] = { .y = 2 },
    [0] = { .x = 1, .y = 1 }
};
```

Designated initializers are a core C99 feature (§6.7.8).  They are also
pervasive in real-world systems code — driver-style struct init, sparse array
tables, and protocol-field initialization all rely on them.  Without them a
large class of real C programs cannot compile.

This is a **language feature stage**.  No driver, lifecycle, or self-host
infrastructure changes.

---

## Background — current initializer machinery

The compiler already handles positional brace initializers (stage 32/43/44):

- `AST_INITIALIZER_LIST` — brace-initializer AST node; children are positional
  values in order.
- `parse_initializer` (`src/parser.c`) — produces an `AST_INITIALIZER_LIST`
  node or falls through to `parse_assignment_expression`.
- `emit_local_struct_init` (`src/codegen.c`) — iterates fields `0..N-1`,
  pairing field `i` with `list->children[i]`.
- Local array init (inside `codegen_statement`) — iterates indices `0..length-1`,
  pairing index `i` with `list->children[i]`, zero-filling beyond the list.
- `emit_global_struct` — iterates fields in declared order; uses
  `list->children[i]` for field `i`.
- Global array emit (inside `codegen_emit_data`) — emits each element from the
  list in order, then trailing zeros.

All of these assume one-to-one positional correspondence between list children
and struct fields / array indices.  Designated initializers break that
assumption: a designator can name any field or index, and subsequent
non-designated elements advance the "current position" sequentially from
there.

---

## Grammar update

Extend the `<initializer_list>` rule to allow each element to carry an
optional designator:

```ebnf
<initializer> ::= <assignment_expression>
                | "{" <initializer_list> [ "," ] "}"

<initializer_list> ::= <initializer_element> { "," <initializer_element> }

<initializer_element> ::= <designator_list> "=" <initializer>
                        | <initializer>

<designator_list> ::= <designator> { <designator> }

<designator> ::= "." IDENTIFIER
               | "[" <constant_integer_expression> "]"
```

The `<constant_integer_expression>` reuses the same grammar as `case` label
expressions (int literals, char literals, enum constants, and `+`/`-`
combinations thereof — `eval_case_const_expr` in `src/parser.c`).

**Out of scope for this stage:** chained designator lists (e.g.
`.a.b`, `.arr[2]`).  Each `<designator_list>` must contain exactly one
designator.  Parsing a second consecutive `.` or `[` after the first, before
the `=`, is a "chained designators not yet supported" error.

---

## Task 1 — New AST node: `AST_DESIGNATED_INIT`

Add to the `ASTNodeType` enum in `include/ast.h` (after
`AST_INITIALIZER_LIST`):

```c
AST_DESIGNATED_INIT,  /* stage 97: designator + value inside an initializer list */
```

Node layout:

| Field | Member designator `.name` | Array index designator `[i]` |
|---|---|---|
| `value` | member name string | `NULL` |
| `byte_length` | 0 (unused) | integer index ≥ 0 |
| `children[0]` | the initializer value | the initializer value |

Discrimination: `node->value != NULL` → member designator;
`node->value == NULL` → array index designator with index in `node->byte_length`.

`child_count` is always 1.  The child is produced by `parse_initializer`
recursively, so it may itself be an `AST_INITIALIZER_LIST` for a nested
brace-initializer.

---

## Task 2 — Parser: detect and parse designators

In `src/parser.c`, split `parse_initializer` so that the brace-element loop
calls a new static helper `parse_initializer_element`:

```c
/* Parse one element of an initializer list: an optional designator followed
 * by '=' and a value, or a plain initializer. */
static ASTNode *parse_initializer_element(Parser *parser);
```

Pseudocode for `parse_initializer_element`:

```
if current == TOKEN_DOT:
    consume '.'
    expect TOKEN_IDENTIFIER → save member_name
    expect TOKEN_ASSIGN ('=')

    /* Reject a second designator — chaining not yet supported. */
    if current == TOKEN_DOT || current == TOKEN_LBRACKET:
        PARSER_ERROR("error: chained designators not yet supported\n")

    ASTNode *node = parser_node(AST_DESIGNATED_INIT, member_name)
    node->byte_length = 0
    ast_add_child(node, parse_initializer(parser))
    return node

else if current == TOKEN_LBRACKET:
    consume '['
    long index = eval_case_const_expr(parser)   /* existing helper */
    if index < 0:
        PARSER_ERROR("error: array designator index must be non-negative\n")
    expect TOKEN_RBRACKET (']')
    expect TOKEN_ASSIGN ('=')

    /* Reject a second designator — chaining not yet supported. */
    if current == TOKEN_DOT || current == TOKEN_LBRACKET:
        PARSER_ERROR("error: chained designators not yet supported\n")

    ASTNode *node = parser_node(AST_DESIGNATED_INIT, NULL)
    node->byte_length = (int)index
    ast_add_child(node, parse_initializer(parser))
    return node

else:
    return parse_initializer(parser)    /* unchanged path */
```

Update `parse_initializer` to call `parse_initializer_element` instead of
`parse_initializer` directly for each list element:

```c
ast_add_child(list, parse_initializer_element(parser));
while (parser->current.type == TOKEN_COMMA) {
    parser->current = lexer_next_token(parser->lexer);
    if (parser->current.type == TOKEN_RBRACE) break;   /* trailing comma */
    ast_add_child(list, parse_initializer_element(parser));
}
```

**`print_ast` output** (for the print_ast test suite): emit
`AST_DESIGNATED_INIT` as e.g.:

```
DESIGNATED_INIT(.y)
  INT_LITERAL(10)
```

or for an index designator:

```
DESIGNATED_INIT([2])
  INT_LITERAL(5)
```

Add a `case AST_DESIGNATED_INIT:` branch to `src/ast_pretty_printer.c`.

---

## Task 3 — Codegen: local struct designated init

Rewrite `emit_local_struct_init` (`src/codegen.c`) to support a "current
field" cursor and member-name lookup.  The caller already zero-fills the
entire struct slot before calling this function; this routine only needs to
overwrite the fields that have initializers.

```
cur_field = 0   /* current sequential position */

for each child in list:

    if child->type == AST_DESIGNATED_INIT && child->value != NULL:
        /* Member designator: find field by name. */
        found = -1
        for i in 0..st->field_count-1:
            if strcmp(st->fields[i].name, child->value) == 0:
                found = i; break
        if found < 0:
            compile_error("error: struct has no member named '%s'\n", child->value)
        cur_field = found
        elem = child->children[0]
    else if child->type == AST_DESIGNATED_INIT && child->value == NULL:
        compile_error("error: array index designator in struct initializer\n")
    else:
        elem = child

    if cur_field >= st->field_count:
        compile_error("error: too many initializers for struct\n")

    f = &st->fields[cur_field]
    foffset = base_offset - f->offset
    fsize = f->full_type ? f->full_type->size : type_kind_bytes(f->kind)

    /* Dispatch to nested init, string-literal path, pointer path, or scalar
     * path — exactly as today, using `elem` instead of `list->children[i]`. */
    ... (unchanged dispatch logic) ...

    cur_field++
```

The zero-fill at the call site (before `emit_local_struct_init`) ensures that
fields skipped by designators or never named remain zero.

---

## Task 4 — Codegen: local array designated init

In `codegen_statement` (inside `AST_DECLARATION` / `TYPE_ARRAY`), replace the
existing `for i in 0..length-1` loop with a two-phase approach:

**Phase 1 — zero-fill the entire array** (unconditionally, regardless of
whether any designators are present):

```c
for (int b = 0; b < size; b++)
    fprintf(cg->output, "    mov byte [rbp - %d], 0\n", offset - b);
```

**Phase 2 — apply initializers using a current-index cursor**:

```c
int cur = 0;
for (int j = 0; j < list->child_count; j++) {
    ASTNode *child = list->children[j];
    ASTNode *elem;

    if (child->type == AST_DESIGNATED_INIT && child->value == NULL) {
        /* Array index designator. */
        int idx = child->byte_length;
        if (idx < 0 || idx >= length)
            compile_error("error: array designator index %d out of bounds\n", idx);
        cur = idx;
        elem = child->children[0];
    } else if (child->type == AST_DESIGNATED_INIT && child->value != NULL) {
        compile_error("error: member designator in array initializer\n");
    } else {
        elem = child;
    }

    if (cur >= length)
        compile_error("error: too many initializers for array '%s'\n", node->value);

    int elem_offset = offset - cur * elem_size;
    /* ... existing dispatch: struct element, plain scalar ... */
    /* (zero-fill not needed — phase 1 already zeroed everything) */

    cur++;
}
```

Remove the `list->child_count > length` early error check — it was a
positional check that no longer applies when designators can reuse positions.
Replace it with the bounds check inside the loop (`cur >= length`).

Note: because phase 1 zero-fills everything up front, the old trailing
zero-fill arm (`i >= list->child_count` branch) in the original loop is gone.

---

## Task 5 — Codegen: global struct designated init

Rewrite `emit_global_struct` to support a current-field cursor and member-name
lookup.  Global initializers are limited to constant expressions (int
literals, char literals, string literals — unchanged restriction).

The function already iterates fields in declared order and emits padding bytes
to maintain correct layout.  With designators, the initializer elements are no
longer positional, so the strategy is:

1. **Build a field-index → element map** before the field-emission loop.
   Allocate a local array `ASTNode *slots[st->field_count]` initialized to
   `NULL`.  Walk the init list:

   ```
   cur = 0
   for each child in list:
       if child is AST_DESIGNATED_INIT (member):
           find field by name → idx
           slots[idx] = child->children[0]
           cur = idx + 1
       else:
           slots[cur] = child
           cur++
   ```

2. **Emit fields in declared order**, using `slots[i]` as the initializer for
   field `i`.  If `slots[i] == NULL`, emit `fsize` zero bytes (existing
   behavior for missing trailing fields, now generalized to any missing field).

This avoids reordering the emitted NASM bytes and naturally handles padding
and alignment as today.

**Bootstrap note:** `ASTNode *slots[st->field_count]` is a VLA.  Since cc99
does not implement VLAs, and the compiler must self-host, use a small fixed
sentinel instead:

```c
/* Maximum fields supported for designated global struct init.
 * Matches the largest struct in the compiler's own source. */
#define MAX_STRUCT_FIELDS_DESIGNATED 64
ASTNode *slots[MAX_STRUCT_FIELDS_DESIGNATED];
```

Emit a `compile_error` if `st->field_count > MAX_STRUCT_FIELDS_DESIGNATED`.
Check and adjust the constant if any compiler source struct has more fields.

---

## Task 6 — Codegen: global array designated init

In `codegen_emit_data`, extend the global array init arm (currently inside
`gv->kind == TYPE_ARRAY && gv->init_node->type == AST_INITIALIZER_LIST`) to
handle `AST_DESIGNATED_INIT` children.

Strategy: build a `slots` array (size = `gv->full_type->length`) of
`ASTNode *`, all `NULL`, then walk the initializer list with a current-index
cursor to fill slots, then emit in index order:

```c
int length = gv->full_type->length;
#define MAX_ARRAY_ELEMS_DESIGNATED 1024
if (length > MAX_ARRAY_ELEMS_DESIGNATED)
    compile_error("error: array too large for designated initializer emission\n");

ASTNode *slots[MAX_ARRAY_ELEMS_DESIGNATED];
for (int s = 0; s < length; s++) slots[s] = NULL;

int cur = 0;
for (int j = 0; j < list->child_count; j++) {
    ASTNode *child = list->children[j];
    if (child->type == AST_DESIGNATED_INIT && child->value == NULL) {
        int idx = child->byte_length;
        if (idx < 0 || idx >= length)
            compile_error("error: array designator index %d out of bounds\n", idx);
        cur = idx;
        slots[cur++] = child->children[0];
    } else if (child->type == AST_DESIGNATED_INIT) {
        compile_error("error: member designator in array initializer\n");
    } else {
        if (cur >= length)
            compile_error("error: too many initializers for array '%s'\n", gv->name);
        slots[cur++] = child;
    }
}
/* Emit length elements: each slot is the existing element handler or zero. */
for (int i = 0; i < length; i++) {
    ASTNode *elem = slots[i];
    /* ... existing dispatch: struct element, int/char literal, pointer, zero ... */
}
```

`MAX_ARRAY_ELEMS_DESIGNATED` of 1024 covers all realistic compile-time tables.
If hit, bump the constant.  Document the cap in a comment.

---

## Task 7 — `print_ast` support

Add `AST_DESIGNATED_INIT` to `src/ast_pretty_printer.c`.  For a member
designator, print `DESIGNATED_INIT(.name)` followed by the child.  For an
index designator, print `DESIGNATED_INIT([N])` followed by the child.
Indentation follows the standard `print_ast_node` depth convention.

---

## Out of scope — do NOT do these in this stage

- **Chained designators** (`.a.b`, `.arr[0]`).  If detected by the parser,
  emit `"error: chained designators not yet supported\n"` and stop.
- **Designated union init** selecting a non-first member.  The current union
  codegen always initializes the first field.  Extending it to pick an
  arbitrary member by name is a separate stage.  For now, a union initializer
  with a designated element is an error (or allowed only if the designator
  names the first field).
- **Non-constant array index expressions**.  Array designator indices must be
  constant integer expressions (same restriction as `case` labels).  Runtime
  expressions inside `[...]` are a parse error.
- **Designated init for block-scope `static` locals**.  Block-scope static
  arrays and structs are already out of scope (stage 71 comment).  No change.
- **Compound literals** `(T){ ... }` — a related but separate feature.
- **`print_tokens` test updates** — no new tokens are introduced; the lexer is
  unchanged.

---

## Bootstrap caveats

The compiler must continue to self-host (C0→C1→C2).  Two patterns to watch:

1. **VLAs** — C0's parser cannot handle runtime-sized local arrays.  All local
   arrays in the new codegen code must use fixed-size constants
   (`MAX_STRUCT_FIELDS_DESIGNATED`, `MAX_ARRAY_ELEMS_DESIGNATED`) as shown
   above.

2. **`*(T **)vec_get(...)` split** — as in prior stages, if any new Vec access
   uses a dereference-cast pattern, split it into two statements.

Run `./build.sh --mode=self-host` before declaring the stage complete.

---

## Tests

### Unit tests (valid)

**Local struct — basic member designators:**
```c
struct Point { int x; int y; };
int main() {
    struct Point p = { .y = 10, .x = 3 };
    return p.x + p.y;   /* expect 13 */
}
```

**Local struct — partial designated (zero fill):**
```c
struct Trio { int a; int b; int c; };
int main() {
    struct Trio t = { .b = 7 };
    return t.a + t.b + t.c;   /* expect 7 */
}
```

**Local struct — mixed designated and plain (current-position advance):**
```c
struct Four { int a; int b; int c; int d; };
int main() {
    /* .b=2, then plain 3 goes to .c, .d stays zero */
    struct Four f = { .b = 2, 3 };
    return f.a + f.b + f.c + f.d;   /* expect 5 */
}
```

**Local array — basic index designators:**
```c
int main() {
    int a[5] = { [2] = 10, [4] = 20 };
    return a[0] + a[1] + a[2] + a[3] + a[4];   /* expect 30 */
}
```

**Local array — designator advances current position:**
```c
int main() {
    int a[6] = { [2] = 5, 6 };
    /* a = {0, 0, 5, 6, 0, 0} */
    return a[2] + a[3];   /* expect 11 */
}
```

**Local array — designator reorders:**
```c
int main() {
    int a[4] = { [3] = 9, [0] = 1 };
    return a[0] + a[1] + a[2] + a[3];   /* expect 10 */
}
```

**Global struct — out-of-order member designators:**
```c
struct Pair { int x; int y; };
struct Pair g = { .y = 5, .x = 3 };
int main() { return g.x + g.y; }   /* expect 8 */
```

**Global array — sparse designation:**
```c
int table[6] = { [0] = 1, [2] = 3, [4] = 5 };
int main() {
    return table[0] + table[1] + table[2] +
           table[3] + table[4] + table[5];   /* expect 9 */
}
```

**Array of structs — index designator with nested struct:**
```c
struct P { int x; int y; };
struct P pts[3] = { [1] = { .y = 2 }, [0] = { .x = 1 } };
int main() {
    return pts[0].x + pts[0].y + pts[1].x + pts[1].y + pts[2].x + pts[2].y;
    /* expect 3 */
}
```

**Char-literal designator index:**
```c
int main() {
    int a[5] = { ['A' - 'A'] = 65 };   /* [0] = 65 */
    return a[0];   /* expect 65 */
}
```

### Unit tests (invalid)

**Unknown member name:**
```c
struct P { int x; };
int main() { struct P p = { .z = 1 }; return 0; }
/* error: struct has no member named 'z' */
```

**Array index designator in struct initializer:**
```c
struct P { int x; };
int main() { struct P p = { [0] = 1 }; return 0; }
/* error: array index designator in struct initializer */
```

**Member designator in array initializer:**
```c
int main() { int a[3] = { .x = 1 }; return 0; }
/* error: member designator in array initializer */
```

**Array index out of bounds:**
```c
int main() { int a[3] = { [5] = 1 }; return 0; }
/* error: array designator index 5 out of bounds */
```

**Chained designators (not yet supported):**
```c
struct Inner { int v; };
struct Outer { struct Inner a; };
int main() { struct Outer o = { .a.v = 1 }; return 0; }
/* error: chained designators not yet supported */
```

### `print_ast` tests

At minimum one test for a member designator and one for an index designator,
verifying the `DESIGNATED_INIT(.name)` / `DESIGNATED_INIT([N])` output format.

### Integration tests

One integration test that combines multi-file compilation (stage 96) with
designated initializers: two source files each defining a global with
designated init, combined into a single binary that exercises both.

---

## Implementation order

1. `include/ast.h` — add `AST_DESIGNATED_INIT` to the enum.
2. `src/ast_pretty_printer.c` — handle `AST_DESIGNATED_INIT` in the printer
   (needed early so `print_ast` tests can be written and run).
3. `src/parser.c` — add `parse_initializer_element`; call it from
   `parse_initializer`.
4. `src/codegen.c` — update `emit_local_struct_init` (Task 3).
5. `src/codegen.c` — update local array init in `codegen_statement` (Task 4).
6. `src/codegen.c` — update `emit_global_struct` (Task 5).
7. `src/codegen.c` — update global array emit in `codegen_emit_data` (Task 6).
8. `src/version.c` — bump `VERSION_STAGE` to `"00970000"`.
9. Tests — add valid, invalid, print_ast, and integration tests.
10. Documentation (see below).

---

## Docs (at stage close, after all tests pass)

- **`README.md`** — add "designated initializers" to the feature summary;
  refresh aggregate test totals.
- **`docs/grammar.md`** — update the `<initializer>` / `<initializer_list>`
  production to include the `<designator>` forms.
- Run the **`update-supplemental-docs`** skill to refresh the feature
  checklist, parse-tree, and status/features snapshots through stage 97.
- **`docs/self-compilation-report.md`** — record the stage-97 self-host run.
- Update **`src/version.c`** (`VERSION_STAGE "00970000"`).
