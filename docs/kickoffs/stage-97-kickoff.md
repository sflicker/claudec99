# Stage 97 Kickoff

**Stage 97:** Designated Initializers

**Spec:** docs/stages/ClaudeC99-spec-stage-97-designated-initializers.md

---

## Summary

Add C99 designated initializers — `.member = value` for struct fields and
`[index] = value` for array elements.  Designators may appear in any order;
subsequent undesignated elements continue sequentially from the last
designator's position; uninitialized fields and elements are zero-filled.

This is a **language feature stage** touching the AST, parser, and code
generator.  No driver, preprocessor, or lifecycle changes.  The self-host
cycle must continue to pass.

---

## AST changes

**Add `AST_DESIGNATED_INIT`** (`include/ast.h`):

A wrapper node placed inside an `AST_INITIALIZER_LIST` when the element has a
designator.

| Field | Member designator `.name` | Array index designator `[i]` |
|---|---|---|
| `value` | member name string | `NULL` |
| `byte_length` | 0 | integer index ≥ 0 |
| `children[0]` | the initializer value | the initializer value |

Discrimination: `node->value != NULL` → member designator; `node->value == NULL`
→ array index designator.

---

## Tokenizer changes

**None.** `TOKEN_DOT` and `TOKEN_LBRACKET` are already tokenized.  No lexer
changes.

---

## Parser changes

**Add `parse_initializer_element`** (`src/parser.c`):

A static helper called in the brace-list loop of `parse_initializer` instead
of calling `parse_initializer` directly for each element.

- If current token is `TOKEN_DOT`: consume `.`, expect `IDENTIFIER`, expect
  `=`, reject a second `./[` (chained — error), recurse into
  `parse_initializer`, return `AST_DESIGNATED_INIT` with `value` = member name.
- If current token is `TOKEN_LBRACKET`: consume `[`, evaluate index with
  `eval_case_const_expr`, expect `]`, expect `=`, reject chained designator,
  recurse, return `AST_DESIGNATED_INIT` with `value = NULL`,
  `byte_length = index`.
- Otherwise: fall through to existing `parse_initializer` call.

---

## Code generation changes

**`emit_local_struct_init`** (`src/codegen.c`):

Replace the positional `children[i]` loop with a `cur_field` cursor.  For each
child: if it is `AST_DESIGNATED_INIT` with a member name, look up the field by
name and set `cur_field`; otherwise advance `cur_field` sequentially.  Emit
the field at `cur_field`, then increment.  The struct slot is already
zero-filled by the caller, so skipped fields are already zero.

**Local array init** (`codegen_statement`, `TYPE_ARRAY` branch):

Two phases:
1. Zero-fill the entire array unconditionally (byte loop over `size` bytes).
2. Walk the initializer list with a `cur` index cursor.  `AST_DESIGNATED_INIT`
   with `value == NULL` sets `cur = child->byte_length`; plain elements advance
   `cur` sequentially.  Emit to `offset - cur * elem_size` and increment.

Remove the old `list->child_count > length` guard (replaced by per-element
bounds checks inside the loop).

**`emit_global_struct`** (`src/codegen.c`):

Before the field-emission loop, build a `slots[MAX_STRUCT_FIELDS_DESIGNATED]`
array of `ASTNode *` (all `NULL`).  Walk the initializer list with a
`cur_field` cursor, filling `slots` by field name or sequentially.  The
existing field-order emission loop then uses `slots[i]` (or zero if `NULL`)
for field `i`.  `MAX_STRUCT_FIELDS_DESIGNATED = 64` (constant — no VLA).

**Global array emit** (`codegen_emit_data`, array branch):

Build a `slots[MAX_ARRAY_ELEMS_DESIGNATED]` array (all `NULL`,
`MAX_ARRAY_ELEMS_DESIGNATED = 1024`).  Walk the list with a `cur` cursor,
filling slots by index or sequentially.  Emit all `length` slots in order;
`NULL` slots emit zero bytes.

---

## Test plan

1. **Local struct — member designators** (basic, partial/zero-fill, mixed
   designated + plain advancing current position).
2. **Local array — index designators** (sparse, current-position advance,
   reordering).
3. **Global struct — out-of-order designators** emitting correct field layout.
4. **Global array — sparse designation** with zero gaps.
5. **Array of structs** with index + member designators combined.
6. **Invalid** — unknown member name, wrong designator kind (array in struct,
   member in array), index out of bounds, chained designators.
7. **`print_ast`** — one member-designator test, one index-designator test.
8. **Integration** — multi-file test combining stage-96 multi-file compilation
   with stage-97 designated initializers.
9. **Full regression** — all 1483 existing tests must pass unchanged.

---

## Implementation order

1. `include/ast.h` — add `AST_DESIGNATED_INIT`.
2. `src/ast_pretty_printer.c` — handle `AST_DESIGNATED_INIT`.
3. `src/parser.c` — add `parse_initializer_element`, wire into
   `parse_initializer`.
4. `src/codegen.c` — update `emit_local_struct_init`.
5. `src/codegen.c` — update local array init in `codegen_statement`.
6. `src/codegen.c` — update `emit_global_struct`.
7. `src/codegen.c` — update global array emit in `codegen_emit_data`.
8. `src/version.c` — bump `VERSION_STAGE` to `"00970000"`.
9. Add tests (valid, invalid, print_ast, integration).
10. Documentation: README, grammar.md, supplemental docs, self-host report.

---

## Ambiguities / Notes

- **Chained designators** (`.a.b`, `.arr[0]`) are out of scope; the parser
  must error if a second `./[` follows the first before the `=`.

- **Union designated init** is out of scope.  The current union path expects at
  most one positional element.  If a union initializer contains a designated
  element naming the first field, it may work incidentally, but no new union
  designated-init codegen is added.

- **VLA restriction**: `emit_global_struct` and the global array path use
  fixed-size `slots[]` arrays because cc99 does not implement VLAs and must
  self-compile.  The constants `MAX_STRUCT_FIELDS_DESIGNATED = 64` and
  `MAX_ARRAY_ELEMS_DESIGNATED = 1024` are generous; verify against the largest
  actual usages in the compiler's own source before finalizing.

- **`*(T **)vec_get(...)` split**: watch for this C0 parser limitation in any
  new Vec accesses; split into two statements as in prior stages.

- **Self-host**: run `./build.sh --mode=self-host` before closing the stage.
  The compiler source itself does not use designated initializers today, so
  C0→C1→C2 is expected to pass without new bootstrap workarounds.
