# Stage 117 — FP Struct Member Read

**Kickoff Document** — Plan before implementation

---

## 1. Summary of the Spec

This is a **codegen-only bug-fix stage** addressing two related issues that cause
silent data corruption when a `double` or `float` struct field is read as an
rvalue:

1. **Bug 1 — Rvalue load uses wrong instruction**: When a struct field of type
   `float` or `double` is read, the codegen emits `mov eax, [rax]` (4-byte
   integer load) or `mov rax, [rax]` (8-byte integer load) instead of the
   correct SSE2 `movss xmm0, [rax]` or `movsd xmm0, [rax]` (floating-point
   loads).

2. **Bug 2 — Type-inference reports TYPE_INT for FP fields**: The
   `expr_result_type()` function uses `promote_kind(f->kind)` on struct fields,
   which maps `TYPE_DOUBLE` and `TYPE_FLOAT` to `TYPE_INT`. This causes binary
   arithmetic on struct member expressions to take the integer path instead of
   the FP path.

3. **Bug 3 — Missing handler for subscript-then-member pattern**: When a member
   access base is an `AST_ARRAY_INDEX` node (`arr[i].x`), both
   `expr_result_type()` and `sizeof_type_of_expr()` fail to look up the field
   type and fall through to `TYPE_INT`.

**Affected access forms:** Direct dot (`s.x`), arrow (`p->x`), and
subscript-then-dot (`arr[i].x`).

**Observed failure:** For a global struct array with `double` fields, arithmetic
like `bodies[j].x - bodies[i].x` emits integer subtract followed by
`cvtsi2sd`, producing wrong results because the loaded value is the low 32
bits of the double (0x00000000 for 1.5 = 0x3FF8000000000000), which converts
to integer 0.

**Scope:** Code generation only. No tokenizer, parser, or AST changes.

**Tests:** 6 new valid tests in `test/valid/structs/` covering direct dot, arrow,
and subscript-then-dot patterns with `double` and `float` fields.

---

## 2. Required Tokenizer Changes

**None.**

---

## 3. Required Parser Changes

**None.**

---

## 4. Required AST Changes

**None.**

---

## 5. Required Code Generation Changes

All changes in `src/codegen.c`:

### Task 1a — FP early-return for AST_MEMBER_ACCESS rvalue (~line 3015)

In `codegen_expression()`, in the `AST_MEMBER_ACCESS` rvalue block, immediately
before the `int sz = type_size(...)` calculation, add:

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

### Task 1b — FP early-return for AST_ARROW_ACCESS rvalue (~line 3049)

In `codegen_expression()`, in the `AST_ARROW_ACCESS` rvalue block, immediately
before the `int sz = type_size(...)` calculation, add the same FP early-return
block as Task 1a.

### Task 2a — FP-aware type inference in AST_MEMBER_ACCESS VAR_REF base (~line 2072)

In `expr_result_type()`, in the `AST_MEMBER_ACCESS` case, in the VAR_REF base
sub-case (direct struct variable), replace:

```c
} else {
    t = (f->kind == TYPE_POINTER) ? TYPE_POINTER
        : promote_kind(f->kind);
    if (f->kind == TYPE_POINTER) node->full_type = f->full_type;
}
```

With:

```c
} else if (type_is_fp(f->kind)) {
    t = f->kind;
} else {
    t = (f->kind == TYPE_POINTER) ? TYPE_POINTER
        : promote_kind(f->kind);
    if (f->kind == TYPE_POINTER) node->full_type = f->full_type;
}
```

### Task 2b — FP-aware type inference in AST_MEMBER_ACCESS DEREF base (~line 2093)

In `expr_result_type()`, in the `AST_MEMBER_ACCESS` case, in the DEREF base
sub-case (pointer dereference), apply the same fix as Task 2a.

### Task 2c — FP-aware type inference in AST_ARROW_ACCESS VAR_REF base (~line 2117)

In `expr_result_type()`, in the `AST_ARROW_ACCESS` case, apply the same FP-aware
fix as Task 2a.

### Task 3a — Add AST_ARRAY_INDEX base handler to expr_result_type() (~line 2120)

In `expr_result_type()`, in the `AST_MEMBER_ACCESS` case, after the existing
VAR_REF and DEREF sub-cases and before the `break`, add:

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

### Task 3b — Add AST_ARRAY_INDEX base handler to sizeof_type_of_expr() (~line 1854)

In `sizeof_type_of_expr()`, in the `AST_MEMBER_ACCESS` case, apply the same
logic as Task 3a (omitting the `node->full_type =` side-effects), and add the
FP-aware `else if (type_is_fp)` branches to the existing VAR_REF and DEREF
sub-cases.

### Task 4 — Version bump

Update `src/version.c` to `VERSION_STAGE = "01170000"`.

---

## 6. Test Plan

### New valid tests (6 total)

All tests should be placed in `test/valid/structs/`.

1. **`test_struct_double_field_arith__4.c`** — Local struct, double field arithmetic
   - Declare local struct with two `double` fields, set to 1.5 and 2.5, add them,
     return (int) result.
   - Expected exit: 4

2. **`test_struct_ptr_double_field_arith__4.c`** — Pointer-to-struct (arrow), double field arithmetic
   - Declare local struct with two `double` fields, create pointer, set via arrow,
     add via arrow, return (int) result.
   - Expected exit: 4

3. **`test_struct_array_double_field__7.c`** — Global struct array, double field arithmetic (subscript-then-member)
   - Declare global static 3-element array of struct with two `double` fields.
     Initialize pts[0] = {1.0, 2.0}, pts[1] = {3.0, 4.0}, pts[2] = {5.0, 6.0}.
     Compute sum = pts[0].x + pts[1].y + pts[2].x = 1 + 4 + 5.
   - Expected exit: 10

4. **`test_struct_array_double_update__42.c`** — Struct array double field updated via addition
   - Declare global static 2-element array with `double` fields.
     Initialize b[0] = {40.0, 2.0}, then update b[0].vx = b[0].vx + b[0].vy.
   - Expected exit: 42

5. **`test_struct_float_field_arith__3.c`** — Local struct, float field arithmetic
   - Declare local struct with two `float` fields, set to 1.5f and 1.5f, add them,
     return (int) result.
   - Expected exit: 3

6. **`test_struct_double_field_sub__42.c`** — Double field subtraction (benchmark pattern)
   - Declare global static 2-element array of struct with two `double` fields.
     Set pairs[0] = {100.0, 58.0}, compute pairs[1].a = pairs[0].a - pairs[0].b.
   - Expected exit: 42

---

## 7. Implementation Order

1. Apply Task 1a (FP early-return for AST_MEMBER_ACCESS rvalue)
2. Apply Task 1b (FP early-return for AST_ARROW_ACCESS rvalue)
3. Apply Tasks 2a–2c (FP-aware type inference for existing bases)
4. Apply Task 3a (AST_ARRAY_INDEX base handler in expr_result_type)
5. Apply Task 3b (AST_ARRAY_INDEX base handler and FP branches in sizeof_type_of_expr)
6. Build (`cmake --build build`)
7. Run `test/valid/run_tests.sh` — all must pass
8. Add 6 new test files to `test/valid/structs/`
9. Run full suite (`./test/run_all_tests.sh`) — all must pass
10. Apply Task 4 (version bump in src/version.c)
11. Self-host (`./build.sh --mode=self-host`) — C0→C1→C2 cycle
12. Update `docs/self-compilation-report.md`
13. Commit with trailer `Co-Authored-By: sgflicker@gmail.com`
14. Delegate README + milestone + transcript to `haiku-stage-artifact-writer`

---

## 8. Notes and Decisions

- **Codegen-only stage:** No ripple effects in tokenizer, parser, or AST. All
  changes are in `src/codegen.c` and `src/version.c`.

- **Early-return pattern:** FP rvalue loads use early-return (identical to the
  existing array member decay pattern), so the integer load path is never
  reached for `float` or `double` fields.

- **AST_ARRAY_INDEX handler:** The subscript-then-member pattern
  (`arr[i].field`) requires walking from the subscript base to find the
  element type, resolving to a struct, and looking up the field. This is a
  new code path that fills a gap in both `expr_result_type()` and
  `sizeof_type_of_expr()`.

- **Bootstrap safety:** The compiler's own source does not use struct members
  of type `double` or `float` in arithmetic expressions. All FP computation
  is done via local `double` variables. The bootstrap cycle is expected to
  produce identical output for all three passes (C0, C1, C2).

- **Write path already works:** The assignment path at ~line 2512 already has
  an `if (type_is_fp(f->kind))` guard that emits `movss [rbx], xmm0` /
  `movsd [rbx], xmm0`. Only the read (rvalue) path is fixed in this stage.
