# ClaudeC99 Stage 101 — Block-Scope Static Arrays and Structs

## Goal

Lift the codegen guard that rejects `TYPE_ARRAY`, `TYPE_STRUCT`, and
`TYPE_UNION` for block-scope static local variables, so patterns like these
compile and run correctly:

```c
void histogram(int bucket) {
    static int counts[8];     /* uninitialized — zero in .bss */
    static int totals[4] = {0, 0, 0, 0};  /* initialized — .data */
    counts[bucket]++;
    totals[bucket % 4]++;
}

void origin_demo() {
    static struct Point org = {1, 2};
    org.x++;
}
```

Static local arrays and structs persist across calls and carry RIP-relative
addressing, exactly like file-scope globals, but with a compiler-generated
private label (`Lstatic_<func>_<n>`).

This is a **codegen-only stage**.  The parser already handles all `static`
local declarations; only `src/codegen.c` and `include/codegen.h` change.

---

## Background — what already works

Block-scope `static` scalars have been supported since stage 71:

```c
void counter() {
    static int n = 0;      /* .data — persists, initialized */
    static long total;     /* .bss  — persists, zero-initialized */
    n++;
}
```

The existing machinery:

- `codegen_statement` SC_STATIC arm generates a unique `Lstatic_<func>_<n>`
  label and registers the variable in `cg->locals` with `is_static = 1` and
  `offset = 0` (not stack-allocated).
- It pushes a `LocalStaticVar` entry into `cg->local_statics`.
- `codegen_emit_local_statics` (called after the function body) emits each
  entry to `.data` (if initialized) or `.bss` (if not).
- Scalar load/store code (`emit_load_global`, `emit_store_global`) already
  respects `lv->is_static` → `[rel static_label]` addressing.

The only blocker is the explicit guard at line ~4279 in `codegen_statement`:

```c
if (node->decl_type == TYPE_ARRAY || node->decl_type == TYPE_STRUCT ||
    node->decl_type == TYPE_UNION) {
    compile_error(
            "error: static local arrays, structs and unions are not yet supported\n");
}
```

Four codegen sites also assume static arrays can never appear (they need
`is_static` branches), and `LocalStaticVar` lacks a field to carry the
brace-initializer `ASTNode` needed for array/struct emission.

---

## Grammar

No changes.  This stage is entirely within `src/codegen.c` and
`include/codegen.h`.

---

## Task 1 — Add `init_node` to `LocalStaticVar`

In `include/codegen.h`, extend the `LocalStaticVar` struct (currently
lines ~35–44) with an `init_node` field:

```c
typedef struct {
    const char *label;
    TypeKind kind;
    Type *full_type;
    int size;
    int is_initialized;
    long init_value;   /* scalars only */
    int is_unsigned;
    ASTNode *init_node; /* array/struct brace-list; NULL for scalars */
} LocalStaticVar;
```

`init_node` is `NULL` for scalars and for any uninitialized (`.bss`) entry.
For initialized arrays and structs it holds the `AST_INITIALIZER_LIST` child
of the declaration node (or an `AST_STRING_LITERAL` for char arrays).

---

## Task 2 — Remove the guard and add array/struct static registration

In `src/codegen.c`, inside `codegen_statement`'s `SC_STATIC` arm:

### 2a — Remove the guard

Delete lines ~4279–4283:

```c
/* DELETE THIS BLOCK */
if (node->decl_type == TYPE_ARRAY || node->decl_type == TYPE_STRUCT ||
    node->decl_type == TYPE_UNION) {
    compile_error(
            "error: static local arrays, structs and unions are not yet supported\n");
}
```

The scalar path that follows (lines ~4284–4335) is unchanged.

### 2b — Add handling for static array locals

After the closing `return;` of the existing scalar block, add:

```
if node->decl_type == TYPE_ARRAY and node->full_type:
    Validate the initializer (if present):
        init_node = node->child_count > 0 ? node->children[0] : NULL
        is_initialized = (init_node != NULL)
        if init_node != NULL:
            Require init_node->type == AST_INITIALIZER_LIST
            OR (full_type->base->kind == TYPE_CHAR and
                init_node->type == AST_STRING_LITERAL)
            otherwise:
                compile_error "error: initializer for block-scope static '%s' "
                              "must be a brace-enclosed constant list\n"

    Generate label:
        snprintf(label, ..., "Lstatic_%s_%d", cg->current_func, cg->label_count++)
        label_ptr = codegen_intern(cg, label)

    Register LocalVar (lv):
        .name       = node->value
        .offset     = 0               /* not stack-allocated */
        .size       = type_kind_bytes(node->full_type->base->kind)
        .kind       = TYPE_ARRAY
        .full_type  = node->full_type
        .is_const   = node->is_const
        .is_unsigned = 0
        .is_static  = 1
        .static_label = label_ptr
        vec_push(&cg->locals, &new_static_lv)

    Push LocalStaticVar (sv):
        .label         = label_ptr
        .kind          = TYPE_ARRAY
        .full_type     = node->full_type
        .size          = node->full_type->size   /* total bytes */
        .is_initialized = is_initialized
        .init_value    = 0
        .is_unsigned   = 0
        .init_node     = init_node
        vec_push(&cg->local_statics, &new_sv)

    return
```

### 2c — Add handling for static struct/union locals

Similarly, after the array block:

```
if (node->decl_type == TYPE_STRUCT || node->decl_type == TYPE_UNION)
   and node->full_type:
    if node->full_type->size == 0:
        compile_error "error: variable '%s' has incomplete %s type\n"

    Validate the initializer (if present):
        init_node = node->child_count > 0 ? node->children[0] : NULL
        is_initialized = (init_node != NULL)
        if init_node != NULL
           and init_node->type != AST_INITIALIZER_LIST:
            compile_error "error: initializer for block-scope static '%s' "
                          "must be a brace-enclosed constant list\n"

    Generate label (same snprintf pattern as above)

    Register LocalVar (lv):
        .name       = node->value
        .offset     = 0
        .size       = node->full_type->size
        .kind       = node->decl_type
        .full_type  = node->full_type
        .is_const   = node->is_const
        .is_unsigned = 0
        .is_static  = 1
        .static_label = label_ptr
        vec_push(&cg->locals, &new_static_lv)

    Push LocalStaticVar (sv):
        .label         = label_ptr
        .kind          = node->decl_type
        .full_type     = node->full_type
        .size          = node->full_type->size
        .is_initialized = is_initialized
        .init_value    = 0
        .is_unsigned   = 0
        .init_node     = init_node
        vec_push(&cg->local_statics, &new_sv)

    return
```

---

## Task 3 — Extend `codegen_emit_local_statics`

`codegen_emit_local_statics` (lines ~5750–5776) currently only handles
scalars.  Extend both the `.data` and `.bss` loops.

### `.data` loop additions (initialized statics)

After the existing scalar emission line:
```c
fprintf(cg->output, "%s: %s %ld\n",
        sv->label, data_init_directive(sv->kind), sv->init_value);
```

Add branches before it (check kind first):

**Char array initialized from string literal:**
```c
if (sv->kind == TYPE_ARRAY && sv->init_node &&
    sv->init_node->type == AST_STRING_LITERAL) {
    ASTNode *str = sv->init_node;
    int arr_len = sv->full_type ? sv->full_type->length
                                : str->byte_length + 1;
    fprintf(cg->output, "%s: db ", sv->label);
    for (int j = 0; j < arr_len; j++) {
        unsigned char b = (j < str->byte_length)
                          ? (unsigned char)str->value[j] : 0;
        if (j > 0) fprintf(cg->output, ", ");
        fprintf(cg->output, "%d", b);
    }
    fprintf(cg->output, "\n");
}
```

**Array initialized from brace-list (`AST_INITIALIZER_LIST`):**

Reuse the same slots-map pattern as the global array emit in
`codegen_emit_data` (lines ~5624–5704).  The logic is identical; only the
label name comes from `sv->label` instead of `gv->name`.  The core steps:

1. Compute `arr_len = sv->full_type->length` and
   `elem_type = sv->full_type->base`.
2. Build `ASTNode *slots[arr_len]` initialized to `NULL`.
3. Walk `sv->init_node->children`, handling `AST_DESIGNATED_INIT` (index
   designators) or plain elements.
4. Emit each slot: `INT_LITERAL` → `dir value`, `CHAR_LITERAL` → `dir value`,
   `NULL` slot → `dir 0`.  Struct-element and string-pointer elements are
   out of scope for this stage (error if encountered).
5. Emit the label prefix `"%s:\n"` before the first element.

**Struct initialized from brace-list:**
```c
if (sv->kind == TYPE_STRUCT && sv->init_node) {
    fprintf(cg->output, "%s:\n", sv->label);
    emit_global_struct(cg, sv->full_type, sv->init_node);
}
```

**Union initialized from brace-list:**

Inline the same union first-member logic used in `codegen_emit_data`
(lines ~5592–5619), replacing `gv->name` with `sv->label` and `gv->full_type`
with `sv->full_type`.

**Scalar fallthrough (unchanged):**  
Only reaches the existing `data_init_directive` line when none of the above
conditions matched.

### `.bss` loop additions (uninitialized statics)

Replace the current single-line emit:
```c
fprintf(cg->output, "%s: %s 1\n", sv->label, bss_res_directive(sv->kind));
```

with a branch:

```c
if (sv->kind == TYPE_ARRAY && sv->full_type) {
    fprintf(cg->output, "%s: %s %d\n",
            sv->label,
            bss_res_directive(sv->full_type->base->kind),
            sv->full_type->length);
} else if ((sv->kind == TYPE_STRUCT || sv->kind == TYPE_UNION) &&
           sv->full_type) {
    fprintf(cg->output, "%s: resb %d\n", sv->label, sv->full_type->size);
} else {
    fprintf(cg->output, "%s: %s 1\n", sv->label, bss_res_directive(sv->kind));
}
```

---

## Task 4 — Array decay in `codegen_expression` (TYPE_ARRAY VAR_REF)

In `src/codegen.c`, inside `codegen_expression`'s `AST_VAR_REF` arm, the
`lv->kind == TYPE_ARRAY` branch (lines ~2068–2073) currently always emits a
stack-relative address:

```c
if (lv->kind == TYPE_ARRAY) {
    fprintf(cg->output, "    lea rax, [rbp - %d]\n", lv->offset);
    node->result_type = TYPE_POINTER;
    node->full_type = type_pointer(lv->full_type->base);
    return;
}
```

Replace with:

```c
if (lv->kind == TYPE_ARRAY) {
    if (lv->is_static)
        fprintf(cg->output, "    lea rax, [rel %s]\n", lv->static_label);
    else
        fprintf(cg->output, "    lea rax, [rbp - %d]\n", lv->offset);
    node->result_type = TYPE_POINTER;
    node->full_type = type_pointer(lv->full_type->base);
    return;
}
```

---

## Task 5 — Array subscript base in `emit_array_index_addr`

In `src/codegen.c`, in `emit_array_index_addr`, the local-array branch
(lines ~985–989) has a stale comment and no `is_static` branch:

```c
if (lv->kind == TYPE_ARRAY) {
    element = lv->full_type->base;
    /* Array statics are rejected at declaration time; is_static is
     * always false here, so no static-label branch needed. */
    fprintf(cg->output, "    lea rax, [rbp - %d]\n", lv->offset);
}
```

Replace with:

```c
if (lv->kind == TYPE_ARRAY) {
    element = lv->full_type->base;
    if (lv->is_static)
        fprintf(cg->output, "    lea rax, [rel %s]\n", lv->static_label);
    else
        fprintf(cg->output, "    lea rax, [rbp - %d]\n", lv->offset);
}
```

(Remove the stale comment.)

---

## Task 6 — Struct member address in `emit_member_addr`

In `src/codegen.c`, in `emit_member_addr`, the local-struct `AST_VAR_REF`
branch (lines ~1177–1192) emits a stack-relative address without checking
`is_static`:

```c
LocalVar *lv = codegen_find_var(cg, base->value);
if (lv) {
    ...
    StructField *f = find_struct_field(lv->full_type, field_name);
    ...
    fprintf(cg->output, "    lea rax, [rbp - %d]\n", lv->offset - f->offset);
    return f;
}
```

For a static struct, `lv->offset` is 0, so `[rbp - 0 + f->offset]` is
nonsensical.  Replace the `fprintf` line with:

```c
if (lv->is_static) {
    if (f->offset != 0)
        fprintf(cg->output, "    lea rax, [rel %s + %d]\n",
                lv->static_label, f->offset);
    else
        fprintf(cg->output, "    lea rax, [rel %s]\n", lv->static_label);
} else {
    fprintf(cg->output, "    lea rax, [rbp - %d]\n", lv->offset - f->offset);
}
```

Note: `emit_struct_addr` (used for whole-struct assignment/argument passing)
already handles `lv->is_static` at lines ~1302–1305 — no change needed there.

---

## Out of scope — do NOT do these in this stage

- **Designated initializers for static arrays and structs** — index
  (`[i] = v`) and member (`.field = v`) designators in the brace-list of a
  static local.  Allowed at file scope and for automatic locals; extending to
  static locals is a separate stage.
- **Multidimensional static arrays** — `static int grid[3][4];` involves
  nested array types.  Single-dimension arrays only.
- **Static arrays of structs** — `static struct Point pts[4];` involves
  struct-element emission in the slots loop.  Plain integer/char element
  arrays only.
- **Runtime initializers for static arrays** — only `AST_INITIALIZER_LIST`
  and `AST_STRING_LITERAL` (for `char` arrays) are accepted.  Function calls,
  variable references, or compound expressions in the initializer → error.
- **`static` compound literals** — `static int *p = (int[]){1,2};` is file-
  scope compound literal territory; out of scope project-wide.
- **Floating-point members** — out of scope project-wide.

---

## Bootstrap caveats

The compiler's own source (lexer, parser, codegen) uses `static` scalars
extensively but no `static` array or struct locals.  The self-host cycle
should pass without changes to the compiler's source code.

Run `./build.sh --mode=self-host` before declaring the stage complete.

---

## Tests

### Valid tests

**Uninitialized static int array persists across calls:**
```c
void bump(int i) {
    static int arr[4];
    arr[i]++;
}
int main() {
    bump(0); bump(0); bump(1);
    /* arr[0] == 2, arr[1] == 1 */
    return 0;
}
```
_(Return 0 by checking no crash; or add a getter function and compare.)_

**Initialized static int array:**
```c
int get_step(int i) {
    static int steps[3] = {10, 20, 30};
    return steps[i];
}
int main() {
    return get_step(0) + get_step(1) + get_step(2) - 60;  /* expect 0 */
}
```

**Uninitialized static struct:**
```c
struct Counter { int n; int total; };
void tick(int v) {
    static struct Counter c;
    c.n++;
    c.total += v;
}
int main() {
    tick(5); tick(3);
    return 0;
}
```

**Initialized static struct:**
```c
struct Point { int x; int y; };
int sum_coords(void) {
    static struct Point p = {3, 4};
    return p.x + p.y;
}
int main() {
    return sum_coords() - 7;  /* expect 0 */
}
```

**Static char array initialized from string literal:**
```c
char *greeting(void) {
    static char msg[] = "hi";
    return msg;
}
int main() {
    char *s = greeting();
    return (s[0] == 'h' && s[1] == 'i' && s[2] == '\0') ? 0 : 1;
}
```

**Static array persists across calls (integration-style):**
```c
int count_calls(void) {
    static int calls[1];
    calls[0]++;
    return calls[0];
}
int main() {
    count_calls(); count_calls();
    return count_calls() - 3;  /* expect 0 */
}
```

### Invalid tests

**Non-constant initializer for static array:**
```c
int n = 5;
int f(void) {
    static int arr[3] = n;   /* error: must be a brace-enclosed constant list */
    return arr[0];
}
int main() { return f(); }
```

**Non-constant initializer for static struct:**
```c
struct P { int x; int y; };
struct P g = {1, 2};
int f(void) {
    static struct P p = g;   /* error: must be a brace-enclosed constant list */
    return p.x;
}
int main() { return f(); }
```

---

## Implementation order

1. `include/codegen.h` — add `ASTNode *init_node` to `LocalStaticVar` (Task 1).
2. `src/codegen.c` — remove guard and add array/struct static registration in
   `codegen_statement` SC_STATIC arm (Task 2).
3. `src/codegen.c` — extend `codegen_emit_local_statics` for arrays/structs
   in `.data` and `.bss` (Task 3).
4. `src/codegen.c` — add `is_static` branch in array VAR_REF in
   `codegen_expression` (Task 4).
5. `src/codegen.c` — add `is_static` branch and remove stale comment in
   `emit_array_index_addr` (Task 5).
6. `src/codegen.c` — add `is_static` check in `emit_member_addr`'s local
   struct branch (Task 6).
7. `src/version.c` — bump `VERSION_STAGE` to `"01010000"`.
8. Tests — add valid and invalid tests as listed above.
9. Documentation (see below).

---

## Docs (at stage close, after all tests pass)

- **`README.md`** — update "Through stage …" line; update the "block-scope
  `static` local variables" bullet to include arrays and structs; refresh test
  totals.
- **`docs/grammar.md`** — no grammar changes; note that codegen now supports
  aggregate static locals in the implementation notes if relevant.
- Run the **`update-supplemental-docs`** skill to refresh the feature
  checklist, parse-tree, and status/features snapshots through stage 101.
- **`docs/self-compilation-report.md`** — record the stage-101 self-host run.
