# Stage 97 Kickoff — Designated Initializers

## Summary

Add C99 designated initializers — the `.member = value` and `[index] = value`
syntactic forms that let brace initializers name their target field or array
index instead of relying on positional order.

```c
struct Point { int x, y; };
struct Point p = { .y = 10, .x = 3 };   /* member designators */

int a[6] = { [2] = 5, [5] = 9 };        /* array index designators */
```

Designated initializers are pervasive in real-world systems code (driver-style
struct init, sparse array tables, protocol-field initialization). Without them,
a large class of real C programs cannot compile.

This is a **language feature stage** touching the AST, parser, and code
generator. No driver, lexer, or lifecycle changes. The self-host cycle must
continue to pass.

---

## Tokenizer Changes

None. Designated initializers use only existing tokens: `.`, `[`, `]`, `=`.

---

## Parser Changes

**Add `parse_initializer_element()`** (`src/parser.c`):

A static helper called in the brace-element loop of `parse_initializer()` instead
of calling `parse_initializer()` directly for each element.

Pseudocode:

```
if current == TOKEN_DOT:
    consume '.'
    expect IDENTIFIER → save member_name
    expect '=' (TOKEN_ASSIGN)
    if current == TOKEN_DOT or TOKEN_LBRACKET:
        error "chained designators not yet supported"
    return AST_DESIGNATED_INIT(.member_name, parse_initializer())

else if current == TOKEN_LBRACKET:
    consume '['
    long index = eval_case_const_expr()  /* forward declare eval_case_const_expr */
    if index < 0:
        error "array designator index must be non-negative"
    expect ']'
    expect '='
    if current == TOKEN_DOT or TOKEN_LBRACKET:
        error "chained designators not yet supported"
    return AST_DESIGNATED_INIT([index], parse_initializer())

else:
    return parse_initializer()   /* unchanged path */
```

Update `parse_initializer()` to call `parse_initializer_element()` in the
brace-list loop instead of `parse_initializer()`.

---

## AST Changes

**Add `AST_DESIGNATED_INIT`** to `ASTNodeType` enum (`include/ast.h`), after
`AST_INITIALIZER_LIST`:

Node layout:

| Field | Member designator `.name` | Array index designator `[i]` |
|-------|---------------------------|------------------------------|
| `value` | member name string | `NULL` |
| `byte_length` | 0 (unused) | integer index ≥ 0 |
| `children[0]` | initializer value | initializer value |
| `child_count` | 1 | 1 |

Discrimination: `node->value != NULL` → member designator; `node->value == NULL`
→ array index designator with index in `node->byte_length`.

---

## Code Generation Changes

### Task 1 — Local struct designated init

**Function:** `emit_local_struct_init()` (`src/codegen.c`)

Replace the positional `children[i]` loop with a `cur_field` cursor and member-name
lookup. For each child:

- If `AST_DESIGNATED_INIT` with `value != NULL`: look up field by name, set
  `cur_field` to that index
- If `AST_DESIGNATED_INIT` with `value == NULL`: error (array designator in struct)
- Otherwise: use `cur_field` as-is

Emit field at `cur_field`, then increment. The struct slot is already
zero-filled by the caller, so skipped fields remain zero.

### Task 2 — Local array designated init

**Location:** `codegen_statement()`, `TYPE_ARRAY` branch (`src/codegen.c`)

Two-phase approach:

**Phase 1 — Zero-fill:** Unconditionally zero-fill the entire array (byte loop).

**Phase 2 — Apply designators:** Walk the initializer list with a `cur` index cursor:

- If `AST_DESIGNATED_INIT` with `value == NULL`: set `cur = child->byte_length`,
  check bounds
- If `AST_DESIGNATED_INIT` with `value != NULL`: error (member designator in array)
- Otherwise: use child as-is

Emit to `offset - cur * elem_size`, then increment. Remove the old
`list->child_count > length` guard (replaced by per-element bounds checks).

### Task 3 — Global struct designated init

**Function:** `emit_global_struct()` (`src/codegen.c`)

Before the field-emission loop, build a slots array and walk the initializer list:

1. Allocate `ASTNode *slots[MAX_STRUCT_FIELDS_DESIGNATED]`, all `NULL`
2. Walk init list with `cur_field` cursor, filling `slots` by field name or sequentially
3. Emit fields in declared order, using `slots[i]` (or zero if `NULL`) for field `i`

`MAX_STRUCT_FIELDS_DESIGNATED = 64` (fixed constant, no VLAs for self-hosting).
Error if `st->field_count > MAX_STRUCT_FIELDS_DESIGNATED`.

### Task 4 — Global array designated init

**Location:** `codegen_emit_data()`, array branch (`src/codegen.c`)

Build a slots array and walk the initializer list:

1. Allocate `ASTNode *slots[MAX_ARRAY_ELEMS_DESIGNATED]`, all `NULL`
2. Walk init list with `cur` cursor, filling `slots` by index or sequentially
3. Emit all `length` slots in order; `NULL` slots emit zero bytes

`MAX_ARRAY_ELEMS_DESIGNATED = 1024` (fixed constant, no VLAs). Error if
`length > MAX_ARRAY_ELEMS_DESIGNATED`.

---

## Test Plan

**Valid tests (9):**
- Local struct: basic member designators
- Local struct: partial designated (zero fill)
- Local struct: mixed designated + plain (advancing current position)
- Local array: basic index designators
- Local array: designator advancing current position
- Local array: designators reordering elements
- Global struct: out-of-order member designators
- Global array: sparse designation
- Array of structs: index + member designators combined
- Char-literal designator index

**Invalid tests (5):**
- Unknown member name
- Array designator in struct initializer
- Member designator in array initializer
- Array index out of bounds
- Chained designators not yet supported

**`print_ast` tests (2):**
- Member designator: `DESIGNATED_INIT(.name)` + child
- Index designator: `DESIGNATED_INIT([N])` + child

**Integration test (1):**
- Multi-file (stage 96) with designated initializers in multiple files

---

## Out of Scope

- Chained designators (`.a.b`, `.arr[0]`)
- Designated union init
- Non-constant array index expressions
- Designated init for block-scope static locals
- Compound literals `(T){ ... }`
- `print_tokens` updates (no new tokens)

---

## Implementation Order

1. `include/ast.h` — add `AST_DESIGNATED_INIT` enum
2. `src/ast_pretty_printer.c` — add case for `AST_DESIGNATED_INIT`
3. `src/parser.c` — add `parse_initializer_element()` helper + forward decl for
   `eval_case_const_expr`; update `parse_initializer()` to call it
4. `src/codegen.c` — rewrite `emit_local_struct_init()` (Task 1)
5. `src/codegen.c` — rewrite local array init in `codegen_statement()` (Task 2)
6. `src/codegen.c` — rewrite `emit_global_struct()` (Task 3)
7. `src/codegen.c` — extend global array emit in `codegen_emit_data()` (Task 4)
8. `src/version.c` — bump `VERSION_STAGE` to `"00970000"`
9. Add tests: valid (9), invalid (5), print_ast (2), integration (1)
10. Documentation: README, grammar.md, supplemental snapshots, self-host report

---

## Key Decisions

- **Fixed-size arrays for slots:** `MAX_STRUCT_FIELDS_DESIGNATED = 64` and
  `MAX_ARRAY_ELEMS_DESIGNATED = 1024` to avoid VLAs (required for self-hosting).
  Verify limits against compiler's own source before finalizing.

- **eval_case_const_expr forward declaration:** Required because it's defined
  after `parse_initializer()` in the source.

- **Two-phase array init:** Phase 1 (zero-fill) + Phase 2 (apply designators)
  simplifies handling of sparse designators and avoids positional assumptions.

- **Slots array strategy:** For both global contexts (struct and array), collect
  designator mappings into a slots array before emission, avoiding reordering
  and maintaining correct padding/alignment behavior.

---

## Ambiguities Resolved

- **Chained designators:** Parse error "chained designators not yet supported"
  if a second `./[` follows the first before `=`.

- **Negative array indices:** Parse error "array designator index must be
  non-negative".

- **Out-of-bounds indices:** Codegen error at emission time for local and global
  contexts.

- **Mixed designator types:** Struct initializer rejects array designators with
  error; array initializer rejects member designators with error.

- **Union designated init:** Out of scope. No new codegen for selecting
  non-first fields by name.

---

## Bootstrap Notes

- **VLA restriction:** C0 parser cannot handle runtime-sized arrays. All new
  codegen code uses fixed-size constants.

- **`*(T **)vec_get(...)` split:** If any new Vec access uses a dereference-cast
  pattern, split into two statements as in prior stages.

- **Self-host verification:** Run `./build.sh --mode=self-host` before closing.
  The compiler's own source does not use designated initializers today, so
  C0→C1→C2 is expected to pass without new bootstrap workarounds.
