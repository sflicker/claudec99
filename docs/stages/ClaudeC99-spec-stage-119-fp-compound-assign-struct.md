# ClaudeC99 Stage 119 — FP Compound Assignment on Struct Members

## Goal

Verify and fix compound assignment (`+=`, `-=`, `*=`, `/=`) on `float` and
`double` struct fields.  Stage 117 fixed reading FP struct fields as rvalues;
the compound-assign path uses the same rvalue load but routes through
`expr_result_type()` to decide whether the binary op is FP or integer.  A
missing `codegen_find_global` fallback in that function causes the **FP
arithmetic path to be silently skipped** when both operands are fields of a
global (file-scope) struct variable, resulting in integer instructions being
emitted on floating-point bit patterns.

### Observed failure

```c
struct Vec2 { double x; double y; };
struct Vec2 g;               /* file-scope (global) */

int main(void) {
    g.x = 3.0;
    g.y = 4.0;
    g.x += g.y;              /* should make g.x == 7.0 */
    return (int)g.x;         /* expected: 7, actual: garbage */
}
```

The exit code is nonsensical because the integer `add` instruction operates on
the raw bit patterns of the IEEE 754 doubles in `eax`/`ecx`.

This is a **codegen-only stage**.  No tokenizer, parser, or AST changes are
needed.

---

## What already works after stage 117

Stage 117 fixed `expr_result_type()` and `sizeof_type_of_expr()` for FP struct
fields, but only for certain base-expression kinds.  The following patterns
work correctly and must continue to pass:

| Pattern | Works? |
|---|---|
| `s.x += 1.0` — local struct, scalar FP RHS | ✓ Stage 117 |
| `s.x += s.y` — local struct, both FP fields | ✓ Stage 117 |
| `p->x += p->y` — local pointer-to-struct, both FP fields | ✓ Stage 117 |
| `arr[i].x += arr[j].y` — subscript-then-dot, both FP | ✓ Stage 117 |
| `g.x += 1.0` — global struct, **scalar** FP literal RHS | ✓ (RHS is known FP; triggers FP path) |

---

## Root-cause analysis

### The compound-assignment desugaring

`a op= b` is desugared at parse time (stage 79):

```
AST_ASSIGNMENT
  children[0]: AST_MEMBER_ACCESS "x"    ← lvalue target
    children[0]: AST_VAR_REF "g"
  children[1]: AST_BINARY_OP "+"
    children[0]: AST_MEMBER_ACCESS "x"  ← cloned rvalue
      children[0]: AST_VAR_REF "g"
    children[1]: AST_MEMBER_ACCESS "y"  ← rvalue
      children[0]: AST_VAR_REF "g"
```

The assignment's LHS path uses `emit_member_addr()` to compute the field
address and then checks `type_is_fp(f->kind)` to emit a `movsd [rbx], xmm0`
store — this already handles global structs correctly (stage 109).

The problem is the RHS binary op.  Before emitting arithmetic code, the FP
arithmetic block asks `expr_result_type()` whether either operand is FP
(line ~4123):

```c
TypeKind fp_lt = expr_result_type(cg, node->children[0]);  /* g.x clone */
TypeKind fp_rt = expr_result_type(cg, node->children[1]);  /* g.y */
int fp_is_arith = ...;
if (fp_is_arith && (type_is_fp(fp_lt) || type_is_fp(fp_rt))) {
    /* FP arithmetic path — SSE2 instructions */
```

If neither `fp_lt` nor `fp_rt` is a floating-point kind the block is skipped
and the integer arithmetic path runs instead, operating on the pointer values
left in `rax` by the preceding `emit_member_addr` call (not on the float
values).

### Bug 1 — `expr_result_type()` `AST_MEMBER_ACCESS` VAR_REF base (line ~2097)

```c
case AST_MEMBER_ACCESS: {
    ASTNode *base_node = node->children[0];
    if (base_node->type == AST_VAR_REF) {
        LocalVar *lv = codegen_find_var(cg, base_node->value);
        if (lv && (lv->kind == TYPE_STRUCT || lv->kind == TYPE_UNION) &&
            lv->full_type) {
            /* ... field resolution ... */
        }
        /* NO FALLBACK to codegen_find_global — if lv is NULL (global var)
         * the block does nothing and t stays at the default TYPE_INT. */
    }
```

For `g.x` where `g` is a file-scope struct, `codegen_find_var` returns `NULL`
because `g` is not on the local-variable stack.  `t` stays `TYPE_INT`, and
`expr_result_type` reports the wrong kind for both operands of `g.x + g.y`.

### Bug 2 — `expr_result_type()` `AST_MEMBER_ACCESS` DEREF base (line ~2120)

The `(*ptr).field` case has the same pattern:

```c
    if (base_node->type == AST_DEREF && ...) {
        LocalVar *plv = codegen_find_var(cg, base_node->children[0]->value);
        if (plv && plv->kind == TYPE_POINTER && ...) { ... }
        /* NO fallback for global pointer-to-struct */
    }
```

Affects `(*gp).x += (*gp).y` where `gp` is a file-scope pointer.

### Bug 3 — `expr_result_type()` `AST_ARROW_ACCESS` (line ~2180)

```c
case AST_ARROW_ACCESS: {
    ASTNode *base_node = node->children[0];
    if (base_node->type == AST_VAR_REF) {
        LocalVar *lv = codegen_find_var(cg, base_node->value);
        if (lv && lv->kind == TYPE_POINTER && ...) { ... }
        /* NO fallback for global pointer-to-struct */
    }
}
```

Affects `gp->x += gp->y` where `gp` is a file-scope pointer-to-struct.

### Bugs 4–5 — same gaps in `sizeof_type_of_expr()` (lines ~1856, ~1912)

`sizeof_type_of_expr()` has identical `AST_MEMBER_ACCESS` VAR_REF and
`AST_ARROW_ACCESS` handlers with the same missing global fallbacks.
`sizeof(g.x)` and `sizeof(gp->x)` return 4 instead of 8 for a `double` field
of a global struct.  These rarely appear directly in user code but the function
is called internally to determine store widths in some codegen helpers.

### Why the bug is hidden when one side is a known-FP scalar

`g.x += 1.0`:
- `expr_result_type(g.x_clone)` → `TYPE_INT` (Bug 1)
- `expr_result_type(1.0)` → `TYPE_DOUBLE` (literal)
- `type_is_fp(TYPE_INT) || type_is_fp(TYPE_DOUBLE)` → **TRUE** → FP path taken
- `codegen_expression(g.x_clone)` correctly emits `movsd xmm0, [rax]` and sets
  `result_type = TYPE_DOUBLE`, so no int→FP conversion is inserted
- Works correctly — the literal's known FP type is enough to force the SSE2 path.

Only when both compound-assign operands are global struct FP members does the
FP path fail to engage.

---

## Task 1 — Fix Bug 1: add global fallback in `expr_result_type()` VAR_REF branch

In `expr_result_type()`, `AST_MEMBER_ACCESS` case, replace the VAR_REF block
(~line 2096):

**Before**:

```c
        if (base_node->type == AST_VAR_REF) {
            LocalVar *lv = codegen_find_var(cg, base_node->value);
            if (lv && (lv->kind == TYPE_STRUCT || lv->kind == TYPE_UNION) &&
                lv->full_type) {
                StructField *f = find_struct_field(lv->full_type, node->value);
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

**After** (factor out `struct_type` and try global when local not found):

```c
        if (base_node->type == AST_VAR_REF) {
            Type *struct_type = NULL;
            LocalVar *lv = codegen_find_var(cg, base_node->value);
            if (lv && (lv->kind == TYPE_STRUCT || lv->kind == TYPE_UNION))
                struct_type = lv->full_type;
            /* Stage 119: fall back to global struct when local not found. */
            if (!struct_type) {
                GlobalVar *gv = codegen_find_global(cg, base_node->value);
                if (gv && (gv->kind == TYPE_STRUCT || gv->kind == TYPE_UNION))
                    struct_type = gv->full_type;
            }
            if (struct_type) {
                StructField *f = find_struct_field(struct_type, node->value);
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

---

## Task 2 — Fix Bug 2: add global fallback in `expr_result_type()` DEREF branch

In `expr_result_type()`, `AST_MEMBER_ACCESS` case, replace the DEREF block
(~line 2118):

**Before**:

```c
        if (base_node->type == AST_DEREF && base_node->child_count > 0 &&
            base_node->children[0]->type == AST_VAR_REF) {
            LocalVar *plv = codegen_find_var(cg, base_node->children[0]->value);
            if (plv && plv->kind == TYPE_POINTER && plv->full_type &&
                plv->full_type->base &&
                (plv->full_type->base->kind == TYPE_STRUCT ||
                 plv->full_type->base->kind == TYPE_UNION)) {
                StructField *f = find_struct_field(plv->full_type->base, node->value);
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

**After**:

```c
        if (base_node->type == AST_DEREF && base_node->child_count > 0 &&
            base_node->children[0]->type == AST_VAR_REF) {
            Type *ptr_base = NULL;
            LocalVar *plv = codegen_find_var(cg, base_node->children[0]->value);
            if (plv && plv->kind == TYPE_POINTER && plv->full_type &&
                plv->full_type->base &&
                (plv->full_type->base->kind == TYPE_STRUCT ||
                 plv->full_type->base->kind == TYPE_UNION))
                ptr_base = plv->full_type->base;
            /* Stage 119: fall back to global pointer-to-struct. */
            if (!ptr_base) {
                GlobalVar *gv = codegen_find_global(cg, base_node->children[0]->value);
                if (gv && gv->kind == TYPE_POINTER && gv->full_type &&
                    gv->full_type->base &&
                    (gv->full_type->base->kind == TYPE_STRUCT ||
                     gv->full_type->base->kind == TYPE_UNION))
                    ptr_base = gv->full_type->base;
            }
            if (ptr_base) {
                StructField *f = find_struct_field(ptr_base, node->value);
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

---

## Task 3 — Fix Bug 3: add global fallback in `expr_result_type()` `AST_ARROW_ACCESS`

In `expr_result_type()`, `AST_ARROW_ACCESS` case (~line 2177):

**Before**:

```c
    case AST_ARROW_ACCESS: {
        ASTNode *base_node = node->children[0];
        if (base_node->type == AST_VAR_REF) {
            LocalVar *lv = codegen_find_var(cg, base_node->value);
            if (lv && lv->kind == TYPE_POINTER && lv->full_type &&
                lv->full_type->base &&
                (lv->full_type->base->kind == TYPE_STRUCT ||
                 lv->full_type->base->kind == TYPE_UNION)) {
                StructField *f = find_struct_field(lv->full_type->base, node->value);
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
        break;
    }
```

**After**:

```c
    case AST_ARROW_ACCESS: {
        ASTNode *base_node = node->children[0];
        if (base_node->type == AST_VAR_REF) {
            Type *ptr_base = NULL;
            LocalVar *lv = codegen_find_var(cg, base_node->value);
            if (lv && lv->kind == TYPE_POINTER && lv->full_type &&
                lv->full_type->base &&
                (lv->full_type->base->kind == TYPE_STRUCT ||
                 lv->full_type->base->kind == TYPE_UNION))
                ptr_base = lv->full_type->base;
            /* Stage 119: fall back to global pointer-to-struct. */
            if (!ptr_base) {
                GlobalVar *gv = codegen_find_global(cg, base_node->value);
                if (gv && gv->kind == TYPE_POINTER && gv->full_type &&
                    gv->full_type->base &&
                    (gv->full_type->base->kind == TYPE_STRUCT ||
                     gv->full_type->base->kind == TYPE_UNION))
                    ptr_base = gv->full_type->base;
            }
            if (ptr_base) {
                StructField *f = find_struct_field(ptr_base, node->value);
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
        break;
    }
```

---

## Task 4 — Fix Bug 4: add global fallback in `sizeof_type_of_expr()` VAR_REF branch

In `sizeof_type_of_expr()`, `AST_MEMBER_ACCESS` case (~line 1856):

**Before**:

```c
    case AST_MEMBER_ACCESS: {
        ASTNode *base = node->children[0];
        if (base->type == AST_VAR_REF) {
            LocalVar *lv = codegen_find_var(cg, base->value);
            if (lv && (lv->kind == TYPE_STRUCT || lv->kind == TYPE_UNION) &&
                lv->full_type) {
                StructField *f = find_struct_field(lv->full_type, node->value);
                if (f) {
                    if (type_is_fp(f->kind)) return f->kind;
                    return f->kind;
                }
            }
        }
```

**After**:

```c
    case AST_MEMBER_ACCESS: {
        ASTNode *base = node->children[0];
        if (base->type == AST_VAR_REF) {
            Type *struct_type = NULL;
            LocalVar *lv = codegen_find_var(cg, base->value);
            if (lv && (lv->kind == TYPE_STRUCT || lv->kind == TYPE_UNION))
                struct_type = lv->full_type;
            /* Stage 119: fall back to global struct when local not found. */
            if (!struct_type) {
                GlobalVar *gv = codegen_find_global(cg, base->value);
                if (gv && (gv->kind == TYPE_STRUCT || gv->kind == TYPE_UNION))
                    struct_type = gv->full_type;
            }
            if (struct_type) {
                StructField *f = find_struct_field(struct_type, node->value);
                if (f) {
                    if (type_is_fp(f->kind)) return f->kind;
                    return f->kind;
                }
            }
        }
```

---

## Task 5 — Fix Bug 5: add global fallback in `sizeof_type_of_expr()` `AST_ARROW_ACCESS`

In `sizeof_type_of_expr()`, `AST_ARROW_ACCESS` case (~line 1910):

**Before**:

```c
    case AST_ARROW_ACCESS: {
        ASTNode *base = node->children[0];
        if (base->type == AST_VAR_REF) {
            LocalVar *lv = codegen_find_var(cg, base->value);
            if (lv && lv->kind == TYPE_POINTER && lv->full_type &&
                lv->full_type->base &&
                (lv->full_type->base->kind == TYPE_STRUCT ||
                 lv->full_type->base->kind == TYPE_UNION)) {
                StructField *f = find_struct_field(lv->full_type->base, node->value);
                if (f) {
                    if (type_is_fp(f->kind)) return f->kind;
                    return f->kind;
                }
            }
        }
        return TYPE_INT;
    }
```

**After**:

```c
    case AST_ARROW_ACCESS: {
        ASTNode *base = node->children[0];
        if (base->type == AST_VAR_REF) {
            Type *ptr_base = NULL;
            LocalVar *lv = codegen_find_var(cg, base->value);
            if (lv && lv->kind == TYPE_POINTER && lv->full_type &&
                lv->full_type->base &&
                (lv->full_type->base->kind == TYPE_STRUCT ||
                 lv->full_type->base->kind == TYPE_UNION))
                ptr_base = lv->full_type->base;
            /* Stage 119: fall back to global pointer-to-struct. */
            if (!ptr_base) {
                GlobalVar *gv = codegen_find_global(cg, base->value);
                if (gv && gv->kind == TYPE_POINTER && gv->full_type &&
                    gv->full_type->base &&
                    (gv->full_type->base->kind == TYPE_STRUCT ||
                     gv->full_type->base->kind == TYPE_UNION))
                    ptr_base = gv->full_type->base;
            }
            if (ptr_base) {
                StructField *f = find_struct_field(ptr_base, node->value);
                if (f) {
                    if (type_is_fp(f->kind)) return f->kind;
                    return f->kind;
                }
            }
        }
        return TYPE_INT;
    }
```

---

## Task 6 — `src/version.c`

Bump `VERSION_STAGE` to `"01190000"`.

---

## Implementation order

1. Apply Task 1 (Bug 1 — `expr_result_type` VAR_REF branch, global fallback).
2. Apply Task 2 (Bug 2 — `expr_result_type` DEREF branch, global fallback).
3. Apply Task 3 (Bug 3 — `expr_result_type` `AST_ARROW_ACCESS`, global fallback).
4. Apply Task 4 (Bug 4 — `sizeof_type_of_expr` VAR_REF branch, global fallback).
5. Apply Task 5 (Bug 5 — `sizeof_type_of_expr` `AST_ARROW_ACCESS`, global fallback).
6. Build (`cmake --build build`).
7. Verify the motivating case exits 7: `struct Vec2 g; g.x=3.0; g.y=4.0; g.x+=g.y; return (int)g.x;`
8. Run `test/valid/run_tests.sh` — all existing tests must pass.
9. Add tests (see below); run full suite (`./test/run_all_tests.sh`).
10. Bump `src/version.c`.
11. Self-host: `./build.sh --mode=self-host` — all three passes must pass.
12. Update `docs/self-compilation-report.md`.
13. Commit.
14. Delegate README + milestone + transcript to `haiku-stage-artifact-writer`.

---

## Out of scope — do NOT do these in this stage

- **`++s.x` / `--p->x` (prefix/postfix increment/decrement on FP struct fields)** —
  `codegen_inc_dec_general` uses integer load/adjust/store regardless of the
  field type; `sz` is computed incorrectly for `TYPE_DOUBLE` (gets 4, not 8).
  This is a separate bug and a separate stage.
- **Compound literals and function-return bases** — e.g., `f().x += 1.0` where
  the base of the member access is a function call.  Not covered.
- **Chained member access** — e.g., `a.b.x += 1.0` where `.b` is a nested
  struct.  The recursive `emit_member_addr` handles the address correctly, but
  `expr_result_type` would need the same global-fallback treatment for the
  inner access.  Not in scope here.
- **Global struct in the `expr_result_type` `AST_MEMBER_ACCESS` DEREF branch in
  `sizeof_type_of_expr`** — the DEREF base (`(*gp).x`) in `sizeof_type_of_expr`
  is not fixed because `sizeof((*gp).x)` is essentially never written; it
  remains a known gap.

---

## Bootstrap caveats

The compiler's own source does not use file-scope struct variables with FP
fields in arithmetic expressions.  All floating-point computation in the
compiler is done via local `double` variables.  The bootstrap cycle is expected
to produce identical output at C0, C1, and C2.

---

## Tests

### Valid tests — place in `test/valid/structs/`

#### Global struct, both fields compound-assign (`+=`)

**`test/valid/structs/test_global_struct_fp_add_assign__7.c`**
```c
struct Vec2 { double x; double y; };
struct Vec2 g;

int main(void) {
    g.x = 3.0;
    g.y = 4.0;
    g.x += g.y;          /* 3.0 + 4.0 = 7.0 */
    return (int)g.x;
}
```
Expected exit: 7

#### Global struct, subtraction compound-assign (`-=`)

**`test/valid/structs/test_global_struct_fp_sub_assign__2.c`**
```c
struct Pair { double a; double b; };
struct Pair p;

int main(void) {
    p.a = 5.0;
    p.b = 3.0;
    p.a -= p.b;           /* 5.0 - 3.0 = 2.0 */
    return (int)p.a;
}
```
Expected exit: 2

#### Global struct, multiply compound-assign (`*=`)

**`test/valid/structs/test_global_struct_fp_mul_assign__6.c`**
```c
struct Scale { double val; double factor; };
struct Scale s;

int main(void) {
    s.val    = 2.0;
    s.factor = 3.0;
    s.val *= s.factor;    /* 2.0 * 3.0 = 6.0 */
    return (int)s.val;
}
```
Expected exit: 6

#### Global pointer-to-struct (arrow), compound-assign

**`test/valid/structs/test_global_ptr_struct_fp_add_assign__9.c`**
```c
struct Vec2 { double x; double y; };
static struct Vec2 node;
static struct Vec2 *gp = &node;      /* file-scope pointer */

int main(void) {
    gp->x = 4.0;
    gp->y = 5.0;
    gp->x += gp->y;      /* 4.0 + 5.0 = 9.0 */
    return (int)gp->x;
}
```
Expected exit: 9

#### Local struct compound-assign still works (regression)

**`test/valid/structs/test_local_struct_fp_add_assign__5.c`**
```c
struct Vec2 { double x; double y; };

int main(void) {
    struct Vec2 v;
    v.x = 2.0;
    v.y = 3.0;
    v.x += v.y;          /* 2.0 + 3.0 = 5.0 */
    return (int)v.x;
}
```
Expected exit: 5

#### Mixed: local struct field plus global struct field

**`test/valid/structs/test_mixed_struct_fp_assign__10.c`**
```c
struct Vec2 { double x; double y; };
struct Vec2 global_v;

int main(void) {
    struct Vec2 local_v;
    global_v.x = 7.0;
    local_v.y  = 3.0;
    local_v.y += global_v.x;   /* 3.0 + 7.0 = 10.0 */
    return (int)local_v.y;
}
```
Expected exit: 10

#### Global struct accumulator pattern

**`test/valid/structs/test_global_struct_fp_accumulate__15.c`**
```c
struct Acc { double sum; double cur; };
static struct Acc acc;

int main(void) {
    int i;
    acc.sum = 0.0;
    for (i = 1; i <= 5; i++) {
        acc.cur  = (double)i;
        acc.sum += acc.cur;   /* 1+2+3+4+5 = 15 */
    }
    return (int)acc.sum;
}
```
Expected exit: 15

---

## Docs (at stage close, after all tests pass)

- **`README.md`** — the FP struct member read/write story is now complete for
  all storage classes; update the capability bullet and test totals.
- **`docs/self-compilation-report.md`** — add stage-119 self-host result.
- Delegate README + milestone + transcript to `haiku-stage-artifact-writer`.
