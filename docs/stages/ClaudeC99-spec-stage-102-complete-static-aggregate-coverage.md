# ClaudeC99 Stage 102 — Complete Static Aggregate Coverage

## Goal

Close the three codegen gaps explicitly deferred from stage 101:

1. **Designated initializers** in block-scope static array initializers
   (`static int arr[4] = {[2] = 99, [0] = 1}`).
2. **Static arrays of structs/unions** — element type is a struct or union
   (`static struct Point pts[3] = {{1,2},{3,4}}`).
3. **Multidimensional static arrays** — the element type is itself an array
   (`static int grid[2][3]`), both uninitialized (`.bss`) and initialized
   (`.data`).

All three are **codegen-only**.  No parser, AST, or grammar changes.

---

## Background — what stage 101 left open

Stage 101 added block-scope static array and struct/union support to
`codegen_emit_local_statics`.  The `.data` loop for arrays (`sv->kind ==
TYPE_ARRAY && sv->init_node->type == AST_INITIALIZER_LIST`) builds a
`slots[]` map and emits each slot, but it has three explicit error guards
that block the patterns addressed here:

**Guard A** (lines ~5893–5897) — designated initializer children:

```c
if (child->type == AST_DESIGNATED_INIT) {
    compile_error(
        "error: designated initializers in block-scope "
        "static array not yet supported\n");
}
```

**Guard B** (lines ~5915–5919) — non-scalar slot elements:

```c
} else {
    compile_error(
        "error: unsupported initializer element in "
        "block-scope static array\n");
}
```

This fires for any slot that is an `AST_INITIALIZER_LIST` (struct-element
row or inner-dimension row of a 2D array).

**Guard C** (lines ~5977–5981) — multidimensional `.bss` emission:

```c
if (sv->kind == TYPE_ARRAY && sv->full_type) {
    fprintf(cg->output, "%s: %s %d\n",
            sv->label,
            bss_res_directive(sv->full_type->base->kind),  /* TYPE_ARRAY → wrong */
            sv->full_type->length);                         /* outer count only  */
}
```

`bss_res_directive(TYPE_ARRAY)` falls through to `resd` (the default), and
`sv->full_type->length` is the outer dimension count alone — both wrong for
a 2D array.  The same bug exists in `codegen_emit_bss` for file-scope
global multidimensional arrays (same pattern, same fix).

---

## Grammar

No changes.  This stage is entirely within `src/codegen.c`.

---

## Task 1 — Designated initializers in static local array initializers

In `codegen_emit_local_statics`'s array brace-list `.data` loop, replace
Guard A with the same handling used by the global array path in
`codegen_emit_data` (lines ~5648–5668):

```c
if (child->type == AST_DESIGNATED_INIT &&
    child->value == NULL) {
    /* Index designator: [N] = value */
    int idx = child->byte_length;
    if (idx < 0 || idx >= arr_len) {
        compile_error(
            "error: array designator index %d out of "
            "bounds\n", idx);
    }
    cur = idx;
    slots[cur++] = child->children[0];
} else if (child->type == AST_DESIGNATED_INIT) {
    /* Member designator in array context */
    compile_error(
        "error: member designator in array initializer\n");
} else {
    if (cur >= arr_len) {
        compile_error(
            "error: too many initializers for static array\n");
    }
    slots[cur++] = child;
}
```

The `child->value == NULL` test distinguishes an index designator
(`AST_DESIGNATED_INIT` with `value = NULL`, index in `byte_length`) from a
member designator (`AST_DESIGNATED_INIT` with `value` = field name).

---

## Task 2 — Static arrays of structs and unions

In the slot-emit loop of the same `.data` path, extend the `if (elem != NULL)`
branch to handle struct-element and union-element slots, and extend the
zero-fill `else` branch accordingly.  The global array path
(`codegen_emit_data`, lines ~5675–5703) already does this correctly;
mirror it exactly:

**Non-NULL slot:**

```c
if (elem_type && elem_type->kind == TYPE_STRUCT &&
    elem->type == AST_INITIALIZER_LIST) {
    emit_global_struct(cg, elem_type, elem);
} else if (elem_type && elem_type->kind == TYPE_UNION &&
           elem->type == AST_INITIALIZER_LIST) {
    /* First-member union init. */
    ... (same inline logic as the sv->kind == TYPE_UNION branch above) ...
} else if (elem->type == AST_INT_LITERAL) {
    ...
} else if (elem->type == AST_CHAR_LITERAL) {
    ...
} else {
    compile_error(
        "error: unsupported initializer element in "
        "block-scope static array\n");
}
```

**NULL slot (zero-fill):**

```c
} else {
    if (elem_type && (elem_type->kind == TYPE_STRUCT ||
                      elem_type->kind == TYPE_UNION)) {
        int b;
        for (b = 0; b < elem_type->size; b++)
            fprintf(cg->output, "    db 0\n");
    } else {
        fprintf(cg->output, "    %s 0\n", dir);
    }
}
```

The union first-member inline logic is:

```c
fprintf(cg->output, "%s:\n", ""); /* label already printed above first elem */
int cur_off = 0;
if (elem->child_count > 1) {
    compile_error("error: too many initializers for union element\n");
}
if (elem->child_count == 1 && elem_type->field_count > 0) {
    StructField *f = &elem_type->fields[0];
    int fsize = f->full_type ? f->full_type->size : type_kind_bytes(f->kind);
    ASTNode *uelem = elem->children[0];
    if (uelem->type == AST_INT_LITERAL) {
        long v = strtol(uelem->value, NULL, 10);
        fprintf(cg->output, "    %s %ld\n", data_init_directive(f->kind), v);
    } else if (uelem->type == AST_CHAR_LITERAL) {
        long v = (long)(unsigned char)uelem->value[0];
        fprintf(cg->output, "    %s %ld\n", data_init_directive(f->kind), v);
    } else {
        compile_error("error: unsupported initializer for union element\n");
    }
    cur_off = fsize;
}
while (cur_off < elem_type->size) {
    fprintf(cg->output, "    db 0\n");
    cur_off++;
}
```

Note: the label (`%s:`) for the whole static array is printed before the
first top-level element (`if (j == 0) fprintf(cg->output, "%s:\n", sv->label)`),
so no per-element label is needed.

---

## Task 3 — Multidimensional static arrays

### 3a — Fix `.bss` emission for multidimensional arrays (local statics)

In `codegen_emit_local_statics`'s `.bss` loop, replace the current array
branch with a two-case branch:

```c
if (sv->kind == TYPE_ARRAY && sv->full_type) {
    if (sv->full_type->base && sv->full_type->base->kind == TYPE_ARRAY) {
        /* Multidimensional: use total byte count. */
        fprintf(cg->output, "%s: resb %d\n", sv->label, sv->full_type->size);
    } else {
        /* Single-dimension: element-directive × length. */
        fprintf(cg->output, "%s: %s %d\n",
                sv->label,
                bss_res_directive(sv->full_type->base->kind),
                sv->full_type->length);
    }
}
```

### 3b — Fix `.bss` emission for multidimensional arrays (file-scope globals)

The same bug exists in `codegen_emit_bss` (lines ~5830–5828):

```c
if (gv->kind == TYPE_ARRAY && gv->full_type) {
    fprintf(cg->output, "%s: %s %d\n",
            gv->name,
            bss_res_directive(gv->full_type->base->kind),
            gv->full_type->length);
}
```

Apply the same two-case fix (using `gv->name`, `gv->full_type`):

```c
if (gv->kind == TYPE_ARRAY && gv->full_type) {
    if (gv->full_type->base && gv->full_type->base->kind == TYPE_ARRAY) {
        fprintf(cg->output, "%s: resb %d\n", gv->name, gv->full_type->size);
    } else {
        fprintf(cg->output, "%s: %s %d\n",
                gv->name,
                bss_res_directive(gv->full_type->base->kind),
                gv->full_type->length);
    }
}
```

### 3c — `.data` emission for initialized multidimensional static arrays

In the array brace-list `.data` loop, after the existing scalar-element
branches and the new struct/union branches from Task 2, add a branch for
inner-dimension rows:

```c
} else if (elem_type && elem_type->kind == TYPE_ARRAY &&
           elem->type == AST_INITIALIZER_LIST) {
    /* Inner dimension row of a 2D array.
     * elem_type is the row type (e.g. int[3] for int[2][3]);
     * emit each scalar element of the row directly. */
    Type *scalar_type = elem_type->base;
    if (scalar_type == NULL || scalar_type->kind == TYPE_ARRAY) {
        compile_error(
            "error: initialized static arrays deeper than 2D "
            "are not yet supported\n");
    }
    const char *row_dir = data_init_directive(scalar_type->kind);
    int row_len = elem_type->length;
    int provided = elem->child_count;
    int ri;
    for (ri = 0; ri < row_len; ri++) {
        if (ri < provided) {
            ASTNode *re = elem->children[ri];
            if (re->type == AST_INT_LITERAL) {
                long v = strtol(re->value, NULL, 10);
                fprintf(cg->output, "    %s %ld\n", row_dir, v);
            } else if (re->type == AST_CHAR_LITERAL) {
                long v = (long)(unsigned char)re->value[0];
                fprintf(cg->output, "    %s %ld\n", row_dir, v);
            } else {
                compile_error(
                    "error: unsupported element in 2D static "
                    "array initializer\n");
            }
        } else {
            fprintf(cg->output, "    %s 0\n", row_dir);
        }
    }
}
```

The zero-fill for a NULL slot in a 2D array:

```c
} else {
    if (elem_type && elem_type->kind == TYPE_ARRAY) {
        /* Zero-fill an entire missing row. */
        int b;
        for (b = 0; b < elem_type->size; b++)
            fprintf(cg->output, "    db 0\n");
    } else if ...
```

---

## Task 4 — `src/version.c`

Bump `VERSION_STAGE` to `"01020000"`.

---

## Implementation order

1. `src/codegen.c` — Task 1: replace Guard A with designated-init handling.
2. `src/codegen.c` — Task 2: extend slot-emit and zero-fill branches for
   struct/union elements.
3. `src/codegen.c` — Task 3a: fix local static multidimensional `.bss`.
4. `src/codegen.c` — Task 3b: fix global multidimensional `.bss`.
5. `src/codegen.c` — Task 3c: add 2D `.data` row emission.
6. `src/version.c` — bump stage.
7. Tests.
8. Documentation.

---

## Out of scope — do NOT do these in this stage

- **3D and deeper static arrays** — `static int cube[2][3][4]` requires
  another level of row-recursion; rejected with the "deeper than 2D" error
  from Task 3c.
- **Designated initializers in 2D static array rows** — `[0][1] = v`
  (chained designators); out of scope project-wide.
- **Static arrays of arrays of structs** — e.g., `static struct P m[2][3]`
  combines 2D and struct-element emission; out of scope for this stage.
- **Designated initializers in static struct initializers** — stage 101
  delegated the whole-struct path to `emit_global_struct`, which already
  handles designated members; this is already working.
- **Union elements beyond the first member** — mirrors the existing
  restriction on file-scope union initialization.

---

## Bootstrap caveats

The compiler's own source uses no static local arrays of structs or
multidimensional static locals.  The self-host cycle should pass without
changes to the compiler's source code.

Run `./build.sh --mode=self-host` before declaring the stage complete.

---

## Tests

### Valid tests

**Designated-init static array:**
```c
int get_slot(int i) {
    static int arr[4] = {[2] = 99, [0] = 1};
    return arr[i];
}
int main() {
    /* arr == {1, 0, 99, 0} */
    return (get_slot(0) == 1 && get_slot(1) == 0 &&
            get_slot(2) == 99 && get_slot(3) == 0) ? 0 : 1;
}
```

**Designated-init static array — out-of-order, persists:**
```c
int bump_and_get(int i) {
    static int tbl[3] = {[1] = 10, [0] = 5};
    tbl[i]++;
    return tbl[i];
}
int main() {
    bump_and_get(0); /* tbl[0] = 6 */
    bump_and_get(1); /* tbl[1] = 11 */
    return (bump_and_get(0) == 7 && bump_and_get(1) == 12) ? 0 : 1;
}
```

**Uninitialized static array of structs:**
```c
struct Pair { int a; int b; };
void set(int i, int a, int b) {
    static struct Pair pts[3];
    pts[i].a = a;
    pts[i].b = b;
}
int get_a(int i) {
    static struct Pair pts[3];
    return pts[i].a;
}
int main() { return 0; }
```
_(Two separate statics; each zeroed.  Return 0 verifies no crash.)_

**Initialized static array of structs:**
```c
struct Point { int x; int y; };
int sum_all(void) {
    static struct Point pts[3] = {{1, 2}, {3, 4}, {5, 6}};
    return pts[0].x + pts[0].y + pts[1].x + pts[1].y +
           pts[2].x + pts[2].y;
}
int main() {
    return sum_all() - 21;  /* 1+2+3+4+5+6 = 21; expect 0 */
}
```

**Uninitialized 2D static array:**
```c
int bump(int r, int c) {
    static int grid[3][4];
    grid[r][c]++;
    return grid[r][c];
}
int main() {
    bump(0, 0); bump(0, 0);
    return (bump(0, 0) == 3 && bump(1, 2) == 1) ? 0 : 1;
}
```

**Initialized 2D static array:**
```c
int cell(int r, int c) {
    static int m[2][3] = {{1, 2, 3}, {4, 5, 6}};
    return m[r][c];
}
int main() {
    return (cell(0, 0) == 1 && cell(0, 2) == 3 &&
            cell(1, 0) == 4 && cell(1, 2) == 6) ? 0 : 1;
}
```

**2D static array persists across calls:**
```c
void record(int r, int c) {
    static int hits[2][2];
    hits[r][c]++;
}
int total(int r, int c) {
    static int hits[2][2];
    return hits[r][c];
}
int main() { return 0; }
```
_(Two separate statics; return 0 verifies no crash.)_

### Invalid tests

**Initialized 3D static array (not yet supported):**
```c
int f(void) {
    static int cube[2][3][4] = {{{1}}};
    return cube[0][0][0];
}
int main() { return f(); }
```
Expected error contains: `deeper than 2D`

---

## Docs (at stage close, after all tests pass)

- **`README.md`** — update "Through stage …" line; extend the block-scope
  `static` local variables bullet to note that static arrays now support
  designated initializers, struct/union element types, and 2D integer arrays;
  refresh test totals.
- **`docs/grammar.md`** — no grammar changes.
- Run the **`update-supplemental-docs`** skill.
- **`docs/self-compilation-report.md`** — record the stage-102 self-host run.
