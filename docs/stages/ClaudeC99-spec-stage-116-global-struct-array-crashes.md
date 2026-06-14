# ClaudeC99 Stage 116 — Global Struct Array Runtime Crashes

## Goal

Fix two codegen bugs that cause silent data corruption or crashes when
global (file-scope) struct arrays are used.  Both bugs were uncovered during
the CC99-BENCH-002 benchmark bring-up effort.

**Bug 1 — BSS size is wrong for single-dimension struct/union arrays.**  
The BSS reserve directive emits `resd N` (N × 4 bytes) instead of
`resb (N × struct_size)` for a 1-D array whose element type is a struct or
union, causing every element beyond the first four bytes to alias unrelated
memory.

**Bug 2 — String literals in `char[N]` sub-arrays emit a pointer.**  
When a 2-D `char[R][C]` global array is initialized from a brace list of
string-literal rows, each row emits `dq Lstr<N>` (an 8-byte pointer) instead
of `C` inline bytes, corrupting layout and producing wrong values.

This is a **codegen-only stage**.  No tokenizer, parser, or AST changes.

---

## Background

### Bug 1 — `bss_res_directive` defaults to `resd` for struct

`bss_res_directive(TypeKind)` maps a type kind to a NASM BSS mnemonic
(`resb`/`resw`/`resd`/`resq`).  For any kind not explicitly listed the
`default:` branch returns `"resd"`:

```c
/* src/codegen.c line 6388 */
static const char *bss_res_directive(TypeKind kind) {
    switch (kind) {
    case TYPE_CHAR:               return "resb";
    case TYPE_SHORT:              return "resw";
    case TYPE_FLOAT:              return "resd";
    case TYPE_LONG_LONG:
    case TYPE_UNSIGNED_LONG_LONG:
    case TYPE_LONG:
    case TYPE_DOUBLE:
    case TYPE_POINTER:            return "resq";
    case TYPE_INT:
    default:                      return "resd";   /* TYPE_STRUCT falls here */
    }
}
```

`TYPE_STRUCT` and `TYPE_UNION` are not enumerated, so they fall through to
`"resd"`.

In `codegen_emit_bss()` the **single-dimension** else-branch calls:

```c
/* src/codegen.c lines 6877-6882 */
} else {
    fprintf(cg->output, "%s: %s %d\n",
            gv->name,
            bss_res_directive(gv->full_type->base->kind),  /* "resd" for struct */
            gv->full_type->length);                        /* element count */
}
```

For `struct Point pts[100]` where `sizeof(Point) == 8` this emits:

```nasm
pts: resd 100      ; 400 bytes — WRONG, need 800 bytes
```

The **multidimensional** branch correctly uses `resb full_type->size`:

```c
/* src/codegen.c lines 6873-6876 — correct */
if (gv->full_type->base->kind == TYPE_ARRAY) {
    fprintf(cg->output, "%s: resb %d\n",
            gv->name, gv->full_type->size);
}
```

The single-dimension branch should use the same `resb full_type->size` form.

The identical bug exists in `codegen_emit_local_statics()` at lines 7143-7148
for block-scope static struct arrays.

### Bug 2 — `AST_STRING_LITERAL` elements always emit `dq Lstr<N>`

In `codegen_emit_data()`, the global array data-emission loop handles each
slot element.  When an element is an `AST_STRING_LITERAL` it unconditionally
adds it to the string pool and emits an 8-byte pointer:

```c
/* src/codegen.c lines 6802-6805 */
} else if (elem->type == AST_STRING_LITERAL) {
    int idx = (int)cg->string_pool.len;
    vec_push(&cg->string_pool, &elem);
    fprintf(cg->output, "    dq Lstr%d\n", idx);   /* always 8-byte pointer */
}
```

This is correct for `char *rows[N] = {"hello", "world"}` (pointer array).
It is wrong for `char rows[N][C] = {"hello", "world"}` (2-D char array),
where each element type is `char[C]` and the row must be stored as `C` inline
bytes (with NUL padding), not as an 8-byte pointer.

The fix must detect the case where `elem_type->kind == TYPE_ARRAY &&
elem_type->base->kind == TYPE_CHAR` and emit the string's bytes directly,
zero-padded to `elem_type->length`.

Similarly, `emit_global_array_elements()` (the recursive helper used for
nested-brace multidimensional arrays) has no `AST_STRING_LITERAL` branch at
all and will hit `compile_error` for the same pattern when called from a
deeper recursion level.

`emit_global_struct()` has a `TYPE_POINTER + AST_STRING_LITERAL` path but no
`TYPE_ARRAY (char[N]) + AST_STRING_LITERAL` path — struct fields of type
`char[N]` initialized from a string literal will also hit `compile_error`.

---

## Task 1 — Fix BSS emission for single-dimension struct/union arrays

### Fix 1a — `codegen_emit_bss()` (~line 6877)

**Before** (the else branch inside the `gv->kind == TYPE_ARRAY` block):

```c
} else {
    fprintf(cg->output, "%s: %s %d\n",
            gv->name,
            bss_res_directive(gv->full_type->base->kind),
            gv->full_type->length);
}
```

**After:**

```c
} else {
    /* Single-dimension array: always use resb × total byte size. */
    fprintf(cg->output, "%s: resb %d\n",
            gv->name, gv->full_type->size);
}
```

`gv->full_type->size` is the total byte size of the entire array
(`element_size × length`), set by the type system when the array type is
built.  Using it means `struct Point pts[100]` (where `sizeof(Point) == 8`)
correctly reserves 800 bytes.

### Fix 1b — `codegen_emit_local_statics()` (~line 7144)

Identical pattern in the local-statics BSS branch:

**Before:**

```c
} else {
    /* Single-dimension: element-directive × length. */
    fprintf(cg->output, "%s: %s %d\n",
            sv->label,
            bss_res_directive(sv->full_type->base->kind),
            sv->full_type->length);
}
```

**After:**

```c
} else {
    /* Single-dimension array: always use resb × total byte size. */
    fprintf(cg->output, "%s: resb %d\n",
            sv->label, sv->full_type->size);
}
```

---

## Task 2 — Fix `char[N]` array element initialization from string literals

Three emission sites must be extended with a `char[N]-from-string` branch.
The pattern for emitting a string literal as inline bytes (from
`codegen_emit_string_pool()`) is:
- emit `s->byte_length` bytes from `s->value[]` as `db` values,
- then emit zero bytes to pad up to `elem_type->length`.

Define a helper function (static, file-scope) to avoid duplicating this
pattern:

```c
/*
 * Emit a string literal's bytes inline as db directives, zero-padded to
 * `field_len` bytes.  Used for char[N] fields and array elements.
 */
static void emit_string_as_bytes(CodeGen *cg, ASTNode *str, int field_len) {
    int byte_len = str->byte_length;  /* excludes NUL */
    int j;
    for (j = 0; j < byte_len && j < field_len; j++)
        fprintf(cg->output, "    db %d\n", (unsigned char)str->value[j]);
    /* Zero-pad to fill the full field width (includes NUL if room). */
    while (j < field_len) {
        fprintf(cg->output, "    db 0\n");
        j++;
    }
}
```

Place this helper just before `emit_global_array_elements()` so both helpers
can use it.

### Fix 2a — `codegen_emit_data()` global array loop (~line 6802)

The existing `AST_STRING_LITERAL` branch must distinguish pointer vs char-array
element type.

**Before:**

```c
} else if (elem->type == AST_STRING_LITERAL) {
    int idx = (int)cg->string_pool.len;
    vec_push(&cg->string_pool, &elem);
    fprintf(cg->output, "    dq Lstr%d\n", idx);
}
```

**After:**

```c
} else if (elem->type == AST_STRING_LITERAL) {
    if (elem_type && elem_type->kind == TYPE_ARRAY &&
        elem_type->base && elem_type->base->kind == TYPE_CHAR) {
        /* char[N] element initialized from string literal — emit bytes inline. */
        emit_string_as_bytes(cg, elem, elem_type->length);
    } else {
        /* Pointer or other type — emit address via string pool. */
        int idx = (int)cg->string_pool.len;
        vec_push(&cg->string_pool, &elem);
        fprintf(cg->output, "    dq Lstr%d\n", idx);
    }
}
```

`elem_type` is already in scope in this function and holds the type of one
element of the outer array.

### Fix 2b — `emit_global_array_elements()` recursive helper (~line 6550)

Add a `char[N]-from-string` branch before the final `compile_error` catch-all:

**Before (the else catch-all):**

```c
} else {
    compile_error("error: unsupported initializer element in "
                  "global array\n");
}
```

**After:**

```c
} else if (elem_type && elem_type->kind == TYPE_ARRAY &&
           elem_type->base && elem_type->base->kind == TYPE_CHAR &&
           elem->type == AST_STRING_LITERAL) {
    /* char[N] sub-array initialized from string literal. */
    emit_string_as_bytes(cg, elem, elem_type->length);
} else {
    compile_error("error: unsupported initializer element in "
                  "global array\n");
}
```

### Fix 2c — `emit_global_struct()` struct field handler (~line 6655)

Inside the `emit_global_struct()` field-emission loop, add a
`char[N]-from-string` branch alongside the existing `TYPE_POINTER` string
branch:

**Before (existing pointer branch + catch-all):**

```c
} else if (f->kind == TYPE_POINTER &&
           elem->type == AST_STRING_LITERAL) {
    int idx = (int)cg->string_pool.len;
    vec_push(&cg->string_pool, &elem);
    fprintf(cg->output, "    dq Lstr%d\n", idx);
} else if (elem->type == AST_INT_LITERAL) {
    ...
} else if (elem->type == AST_CHAR_LITERAL) {
    ...
} else {
    compile_error(
        "error: unsupported initializer for struct field '%s'\n",
        f->name);
}
```

**After:**

```c
} else if (f->kind == TYPE_POINTER &&
           elem->type == AST_STRING_LITERAL) {
    int idx = (int)cg->string_pool.len;
    vec_push(&cg->string_pool, &elem);
    fprintf(cg->output, "    dq Lstr%d\n", idx);
} else if (f->kind == TYPE_ARRAY && f->full_type &&
           f->full_type->base && f->full_type->base->kind == TYPE_CHAR &&
           elem->type == AST_STRING_LITERAL) {
    /* char[N] struct field initialized from string literal. */
    emit_string_as_bytes(cg, elem, f->full_type->length);
} else if (elem->type == AST_INT_LITERAL) {
    ...
} else if (elem->type == AST_CHAR_LITERAL) {
    ...
} else {
    compile_error(
        "error: unsupported initializer for struct field '%s'\n",
        f->name);
}
```

---

## Task 3 — `src/version.c`

Bump `VERSION_STAGE` to `"01160000"`.

---

## Implementation order

1. Add `emit_string_as_bytes()` helper just before `emit_global_array_elements()`.
2. Apply Fix 1a (`codegen_emit_bss()` single-dimension else branch).
3. Apply Fix 1b (`codegen_emit_local_statics()` single-dimension else branch).
4. Apply Fix 2a (`codegen_emit_data()` global array AST_STRING_LITERAL branch).
5. Apply Fix 2b (`emit_global_array_elements()` catch-all before compile_error).
6. Apply Fix 2c (`emit_global_struct()` char[N] string field branch).
7. Build (`cmake --build build`).
8. Run `test/valid/run_tests.sh` — all must pass.
9. Add tests (see below).
10. Run full suite (`./test/run_all_tests.sh`) — all must pass.
11. Bump `src/version.c`.
12. Self-host: `./build.sh --mode=self-host` — all three passes must pass.
13. Update `docs/self-compilation-report.md`.
14. Commit.
15. Delegate README + milestone + transcript to `haiku-stage-artifact-writer`.

---

## Out of scope — do NOT do these in this stage

- **Local (stack-allocated) struct arrays** — these are already emitted
  correctly via `lea` / `mov` sequences targeting `[rbp - offset]`.
- **Initialized global struct arrays with `designated initializers`** — the
  designated-init path in Stage 97 uses a different code path; leave for a
  later stage if gaps exist.
- **`char[]` field in a local struct** — local structs use stack initialization
  via `codegen_local_initializer`, which already has separate byte-emission
  logic.
- **VLAs and pointer-to-char-array** — not targeted here.
- **Shrinking `bss_res_directive()`** — the function is still needed for
  scalar array elements (e.g. `int arr[N]`, `char arr[N]`); do not remove it.
  Only the struct/union array path is fixed by avoiding its call.

---

## Bootstrap caveats

The compiler's own source uses global struct arrays (e.g. `StructField fields[]`
stored inside heap-allocated types, not BSS).  File-scope uninitialized struct
arrays in the compiler source tend to be zero-length or avoided, so this bug
has not triggered during self-hosting.  After the fix the bootstrap cycle is
expected to produce identical object code for the compiler itself.

The char[N] string-init pattern does not appear in the compiler's own source
(it uses `char *` pointers to string literals, not `char[][N]` arrays), so
Fix 2 changes no self-host behavior either.

---

## Tests

Place new tests under the appropriate category in `test/valid/`.

### Valid — uninitialized global struct array (BSS fix)

**`test/valid/arrays/test_global_struct_array_bss__10.c`**
```c
struct Point { int x; int y; };
struct Point pts[3];
int main(void) {
    pts[0].x = 1; pts[0].y = 2;
    pts[1].x = 3; pts[1].y = 4;
    pts[2].x = 5; pts[2].y = 6;
    return pts[0].x + pts[1].y + pts[2].x;  /* 1 + 4 + 5 = 10 */
}
```
Expected exit: 10

### Valid — uninitialized global struct array, larger size

**`test/valid/arrays/test_global_struct_array_bss_large__42.c`**
```c
struct Item { int a; int b; int c; int d; };  /* 16 bytes each */
struct Item items[100];
int main(void) {
    items[99].d = 42;
    return items[99].d;
}
```
Expected exit: 42

### Valid — uninitialized block-scope static struct array (BSS fix 1b)

**`test/valid/arrays/test_static_struct_array_bss__7.c`**
```c
struct Pair { int x; int y; };
int sum(void) {
    static struct Pair data[3];
    data[0].x = 1; data[1].x = 2; data[2].x = 4;
    return data[0].x + data[1].x + data[2].x;
}
int main(void) { return sum(); }
```
Expected exit: 7

### Valid — initialized global struct array (data section correctness check)

**`test/valid/arrays/test_global_struct_array_init__10.c`**
```c
struct Point { int x; int y; };
struct Point pts[3] = {{1,2},{3,4},{5,6}};
int main(void) {
    return pts[0].x + pts[1].y + pts[2].x;  /* 1 + 4 + 5 = 10 */
}
```
Expected exit: 10

### Valid — 2-D char array initialized from string literals (Bug 2)

**`test/valid/arrays/test_global_char2d_string_init__65.c`**
```c
char words[3][8] = {"hello", "world", "A"};
int main(void) {
    /* 'h'=104, 'w'=119, 'A'=65 — return first char of third string */
    return (int)(unsigned char)words[2][0];
}
```
Expected exit: 65

### Valid — 2-D char array: read second row

**`test/valid/arrays/test_global_char2d_row_access__119.c`**
```c
char rows[2][16] = {"hello", "world"};
int main(void) {
    return (int)(unsigned char)rows[1][0];  /* 'w' = 119 */
}
```
Expected exit: 119

### Valid — struct with char[N] field initialized from string literal (Fix 2c)

**`test/valid/structs/test_struct_char_array_field_init__104.c`**
```c
struct Entry { int id; char name[12]; };
struct Entry e = {1, "hello"};
int main(void) {
    return (int)(unsigned char)e.name[0];  /* 'h' = 104 */
}
```
Expected exit: 104

---

## Docs (at stage close, after all tests pass)

- **`README.md`** — add "Through stage 116…" capability entry; update test totals.
- **`docs/self-compilation-report.md`** — add stage-116 self-host result.
- Delegate README + milestone + transcript to `haiku-stage-artifact-writer`.
