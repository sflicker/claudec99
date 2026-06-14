# ClaudeC99 Stage 117 — FP Struct Member Read

## Goal

Fix silent data corruption when a `double` or `float` struct field is read as
an rvalue.  The field's address is computed correctly, but the **load
instruction** emitted is `mov eax, [rax]` (4-byte integer) instead of
`movsd xmm0, [rax]` (8-byte double) or `movss xmm0, [rax]` (4-byte float).
Simultaneously, the **type-inference function** that decides whether binary
arithmetic should use SSE2 or integer instructions reports `TYPE_INT` for
any struct member regardless of the field's declared type, so FP binary ops on
struct fields fall through to the integer arithmetic path.

Both bugs are present for all member-access forms: direct dot (`s.x`), arrow
(`p->x`), and subscript-then-dot (`arr[i].x`).

### Observed failure

```c
struct Body { double x; double y; double vx; double vy; double mass; };
static struct Body bodies[96];

/* bodies[j].x - bodies[i].x  emits integer subtract, then cvtsi2sd.
   GCC exit code: 34.  ClaudeC99 exit code: 0.  */
```

Every field read in every computation is wrong; the returned integer is 0
because the low 32 bits of `double 1.5` (= `0x3FF8000000000000`) is
`0x00000000`, which converts to the integer 0.

This is a **codegen-only stage**.  No tokenizer, parser, or AST changes.

---

## Root-cause analysis

### Bug 1 — Rvalue load uses wrong instruction for FP fields

`codegen_expression()` contains two symmetric blocks that generate rvalue code
for a struct member access and then load the value.  Both call
`emit_member_addr()` or `emit_arrow_addr()` to put the field's address in
`rax`, then switch on the field's byte size to decide the load instruction:

```c
/* src/codegen.c ~line 3015 (AST_MEMBER_ACCESS rvalue block) */
int sz = type_size(f->full_type ? f->full_type : NULL);
if (sz == 0) {
    switch (f->kind) {
    case TYPE_CHAR:  sz = 1; break;
    case TYPE_SHORT: sz = 2; break;
    case TYPE_LONG_LONG:
    case TYPE_UNSIGNED_LONG_LONG:
    case TYPE_LONG:
    case TYPE_POINTER: sz = 8; break;
    default: sz = 4; break;  /* TYPE_DOUBLE and TYPE_FLOAT fall here */
    }
}
switch (sz) {
case 1: fprintf(cg->output, "    movsx eax, byte [rax]\n"); break;
case 2: fprintf(cg->output, "    movsx eax, word [rax]\n"); break;
case 8: fprintf(cg->output, "    mov rax, [rax]\n"); break;
case 4:
default: fprintf(cg->output, "    mov eax, [rax]\n"); break;  /* WRONG for double/float */
}
node->result_type = (f->kind == TYPE_POINTER) ? TYPE_POINTER
                    : promote_kind(f->kind);   /* promote_kind(TYPE_DOUBLE) = TYPE_INT */
```

`TYPE_DOUBLE` is 8 bytes, not 4 — but even if `sz` were corrected to 8 the
`mov rax, [rax]` branch would still be wrong because the value must land in
`xmm0` for the FP arithmetic path.  An early-return FP branch (identical to
the one already present for `AST_ARRAY_INDEX` rvalue at lines 2982–2988) is
the correct fix.

The identical defect is present in the `AST_ARROW_ACCESS` rvalue block at
~line 3049.

### Bug 2 — `expr_result_type()` reports `TYPE_INT` for FP struct fields

`expr_result_type()` decides which code path binary operators take.  Its
`AST_MEMBER_ACCESS` case (line ~2059) handles two base forms (direct struct
variable and simple deref), and both use:

```c
t = (f->kind == TYPE_POINTER) ? TYPE_POINTER : promote_kind(f->kind);
```

`promote_kind` maps only `TYPE_LONG{_LONG}` and `TYPE_UNSIGNED_LONG_LONG` to
`TYPE_LONG`; everything else becomes `TYPE_INT`.  So
`promote_kind(TYPE_DOUBLE)` = `TYPE_INT`, which causes the binary op dispatch
to choose the integer arithmetic path for `a.x - b.x`.

### Bug 3 — `expr_result_type()` falls through for subscript-then-member

When the base of a member access is an `AST_ARRAY_INDEX` node (`bodies[j].x`),
neither existing condition in the `AST_MEMBER_ACCESS` case matches.  The
`break` at the end of the case is reached with `t` still at its default
`TYPE_INT`.  The field's type is never consulted.

The same gap exists in the parallel `AST_ARROW_ACCESS` case and in
`sizeof_type_of_expr()`.

### Interaction between bugs

Bugs 2 and 3 cause the FP binary operator block to be skipped entirely for
`bodies[j].x - bodies[i].x` (both sides report TYPE_INT → integer path).
Bug 1 causes the value to be loaded as an integer even when the binary op
_does_ take the FP path (because one of the other operand is a known-double
local, as in `bodies[i].vx + ax * 0.0002`).  Both bugs must be fixed
together.

Note that **writing** to an FP struct field already works correctly: the
assignment path at line ~2512 has an existing `if (type_is_fp(f->kind))` guard
that emits `movss [rbx], xmm0` / `movsd [rbx], xmm0`.  Only the read (rvalue)
path is broken.

---

## Task 1 — Fix Bug 1: rvalue FP load for `AST_MEMBER_ACCESS` (~line 3015)

In `codegen_expression()`, immediately after the `TYPE_ARRAY` early-return
block and before the `int sz = type_size(...)` calculation, add:

**Before (`int sz = ...` block)**:

```c
        int sz = type_size(f->full_type ? f->full_type : NULL);
        if (sz == 0) {
            switch (f->kind) {
            case TYPE_CHAR:  sz = 1; break;
            ...
```

**After (insert FP early-return first)**:

```c
        if (f->kind == TYPE_FLOAT) {
            fprintf(cg->output, "    movss xmm0, [rax]\n");
            node->result_type = TYPE_FLOAT;
            return;
        }
        if (f->kind == TYPE_DOUBLE) {
            fprintf(cg->output, "    movsd xmm0, [rax]\n");
            node->result_type = TYPE_DOUBLE;
            return;
        }
        int sz = type_size(f->full_type ? f->full_type : NULL);
        ...
```

The existing `node->result_type = promote_kind(f->kind)` line at the bottom of
this block remains unchanged (it is only reached for integer and pointer
fields).

### Task 1b — Same fix for `AST_ARROW_ACCESS` (~line 3049)

The `AST_ARROW_ACCESS` rvalue block has the identical structure.  Apply the
same two-line FP early-return immediately before its `int sz = ...` line:

```c
        if (f->kind == TYPE_FLOAT) {
            fprintf(cg->output, "    movss xmm0, [rax]\n");
            node->result_type = TYPE_FLOAT;
            return;
        }
        if (f->kind == TYPE_DOUBLE) {
            fprintf(cg->output, "    movsd xmm0, [rax]\n");
            node->result_type = TYPE_DOUBLE;
            return;
        }
```

---

## Task 2 — Fix Bug 2: FP-aware type result in `expr_result_type()`

### 2a — `AST_MEMBER_ACCESS` VAR_REF base (~line 2072)

**Before** (inside the `if (f) {` block after the `TYPE_ARRAY` sub-case):

```c
                    } else {
                        t = (f->kind == TYPE_POINTER) ? TYPE_POINTER
                            : promote_kind(f->kind);
                        if (f->kind == TYPE_POINTER) node->full_type = f->full_type;
                    }
```

**After**:

```c
                    } else if (type_is_fp(f->kind)) {
                        t = f->kind;
                    } else {
                        t = (f->kind == TYPE_POINTER) ? TYPE_POINTER
                            : promote_kind(f->kind);
                        if (f->kind == TYPE_POINTER) node->full_type = f->full_type;
                    }
```

### 2b — `AST_MEMBER_ACCESS` DEREF base (~line 2093)

Same fix applied to the parallel `else` branch inside the `if (base_node->type == AST_DEREF ...)` block.

### 2c — `AST_ARROW_ACCESS` VAR_REF base (~line 2117)

Same fix inside the `AST_ARROW_ACCESS` case.

---

## Task 3 — Fix Bug 3: add `AST_ARRAY_INDEX` base to `expr_result_type()`

Inside the `AST_MEMBER_ACCESS` case in `expr_result_type()`, after the existing
two `if (base_node->type == ...)` blocks and before the `break`, add:

```c
        /* arr[i].field — base is a subscript expression. */
        if (base_node->type == AST_ARRAY_INDEX && base_node->child_count > 0) {
            ASTNode *arr_base = base_node->children[0];
            Type *elem_type = NULL;
            if (arr_base->type == AST_VAR_REF) {
                LocalVar *lv = codegen_find_var(cg, arr_base->value);
                if (lv && lv->full_type &&
                    (lv->kind == TYPE_ARRAY || lv->kind == TYPE_POINTER))
                    elem_type = lv->full_type->base;
                if (!elem_type) {
                    GlobalVar *gv = codegen_find_global(cg, arr_base->value);
                    if (gv && gv->full_type &&
                        (gv->kind == TYPE_ARRAY || gv->kind == TYPE_POINTER))
                        elem_type = gv->full_type->base;
                }
            }
            if (elem_type &&
                (elem_type->kind == TYPE_STRUCT || elem_type->kind == TYPE_UNION)) {
                StructField *f = find_struct_field(elem_type, node->value);
                if (f) {
                    if (f->kind == TYPE_ARRAY && f->full_type && f->full_type->base) {
                        t = TYPE_POINTER;
                        node->full_type = type_pointer(f->full_type->base);
                    } else if (type_is_fp(f->kind)) {
                        t = f->kind;
                    } else {
                        t = (f->kind == TYPE_POINTER) ? TYPE_POINTER
                            : promote_kind(f->kind);
                        if (f->kind == TYPE_POINTER) node->full_type = f->full_type;
                    }
                }
            }
        }
```

### 3b — Same addition to `sizeof_type_of_expr()`

`sizeof_type_of_expr()` has an identical `AST_MEMBER_ACCESS` case (~line 1854)
that also lacks the `AST_ARRAY_INDEX` base case.  Apply the same block there
(without the `node->full_type =` side-effects, since `sizeof_type_of_expr`
does not mutate the AST).  Also add the FP-aware `else if (type_is_fp)`
branch to the two existing base cases in that function.

---

## Task 4 — `src/version.c`

Bump `VERSION_STAGE` to `"01170000"`.

---

## Implementation order

1. Apply Task 1a (Bug 1 fix — `AST_MEMBER_ACCESS` rvalue FP load).
2. Apply Task 1b (Bug 1 fix — `AST_ARROW_ACCESS` rvalue FP load).
3. Apply Task 2a–2c (Bug 2 fix — FP-aware type inference, existing bases).
4. Apply Task 3a (Bug 3 fix — `AST_ARRAY_INDEX` base in `expr_result_type`).
5. Apply Task 3b (Bug 3 fix — same in `sizeof_type_of_expr`).
6. Build (`cmake --build build`).
7. Verify the benchmark exits 34: compile `/tmp/bench117.c`, run.
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

- **Float and double struct fields stored via `volatile`** — `volatile` reads
  currently bypass the member-access path entirely; not targeted here.
- **FP fields in bit-fields** — bit-fields are undefined behavior with FP types
  in C99; no action needed.
- **Complex member-access chains with non-VAR_REF bases beyond subscript**
  (e.g. `f()` returning a struct by value, then `.field`) — the fix in
  Task 3 covers the `AST_ARRAY_INDEX` base, which is the only form tested
  here.  Deeper chains remain TYPE_INT in edge cases and can be addressed in a
  later stage.
- **Global single-struct (non-array) FP fields** — `struct S g; g.x + 1.0`
  falls through the existing VAR_REF base path in `expr_result_type`.  This
  is covered by Task 2a (Bug 2 fix), not a separate stage.
- **`sizeof(s.x)` correctness** — `sizeof_type_of_expr` is fixed in Task 3b
  for consistency, but no `sizeof` test is added; it is very unlikely to affect
  any existing test.

---

## Bootstrap caveats

The compiler's own source code does not use struct members of type `double`
or `float` in arithmetic expressions.  All FP computation in the compiler is
done via local `double` variables, not struct fields.  The bootstrap cycle is
expected to produce identical output to the pre-fix compiler for all three
passes (C0, C1, C2).

---

## Tests

Place new tests in `test/valid/` under the appropriate category.

### Valid — local struct, double field arithmetic

**`test/valid/structs/test_struct_double_field_arith__4.c`**
```c
struct Vec2 { double x; double y; };
int main(void) {
    struct Vec2 v;
    v.x = 1.5;
    v.y = 2.5;
    double z = v.x + v.y;
    return (int)z;   /* 4 */
}
```
Expected exit: 4

### Valid — pointer-to-struct (arrow), double field arithmetic

**`test/valid/structs/test_struct_ptr_double_field_arith__4.c`**
```c
struct Vec2 { double x; double y; };
int main(void) {
    struct Vec2 v;
    struct Vec2 *p = &v;
    p->x = 1.5;
    p->y = 2.5;
    double z = p->x + p->y;
    return (int)z;   /* 4 */
}
```
Expected exit: 4

### Valid — global struct array, double field arithmetic (subscript-then-member)

**`test/valid/structs/test_struct_array_double_field__7.c`**
```c
struct Point { double x; double y; };
static struct Point pts[3];
int main(void) {
    pts[0].x = 1.0; pts[0].y = 2.0;
    pts[1].x = 3.0; pts[1].y = 4.0;
    pts[2].x = 5.0; pts[2].y = 6.0;
    double sum = pts[0].x + pts[1].y + pts[2].x;
    return (int)sum;   /* 1 + 4 + 5 = 10 */
}
```
Expected exit: 10

### Valid — struct array double field updated via addition

**`test/valid/structs/test_struct_array_double_update__42.c`**
```c
struct Body { double vx; double vy; };
static struct Body b[2];
int main(void) {
    b[0].vx = 40.0;
    b[0].vy = 2.0;
    b[0].vx = b[0].vx + b[0].vy;
    return (int)b[0].vx;   /* 42 */
}
```
Expected exit: 42

### Valid — local struct, float field arithmetic

**`test/valid/structs/test_struct_float_field_arith__3.c`**
```c
struct Vec2f { float x; float y; };
int main(void) {
    struct Vec2f v;
    v.x = 1.5f;
    v.y = 1.5f;
    float z = v.x + v.y;
    return (int)z;   /* 3 */
}
```
Expected exit: 3

### Valid — double field subtraction (the benchmark pattern)

**`test/valid/structs/test_struct_double_field_sub__42.c`**
```c
struct Pair { double a; double b; };
static struct Pair pairs[2];
int main(void) {
    pairs[0].a = 100.0;
    pairs[0].b = 58.0;
    pairs[1].a = pairs[0].a - pairs[0].b;
    return (int)pairs[1].a;   /* 42 */
}
```
Expected exit: 42

---

## Docs (at stage close, after all tests pass)

- **`README.md`** — add "Through stage 117…" capability entry; update test totals.
- **`docs/self-compilation-report.md`** — add stage-117 self-host result.
- Delegate README + milestone + transcript to `haiku-stage-artifact-writer`.
