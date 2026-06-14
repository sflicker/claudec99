# ClaudeC99 Stage 120 — FP Increment/Decrement on Struct Members

## Goal

Add support for prefix and postfix `++`/`--` on `float` and `double` struct
fields accessed via dot (`.`) and arrow (`->`) operators.

Stage 119 fixed compound assignment (`s.x += delta`, `p->x *= scale`) on FP
struct members.  That stage's spec explicitly deferred `++s.x` and `--p->x`
because the bug lives in a different function — `codegen_inc_dec_general` — and
requires adding SSE2 instructions and two new `.rodata` constants.

### Observed failure

```c
struct Vec { double x; double y; };

int main(void) {
    struct Vec v;
    v.x = 2.0;
    ++v.x;           /* should make v.x == 3.0 */
    return (int)v.x; /* expected: 3, actual: garbage */
}
```

The exit code is nonsensical because the increment emits `mov eax, [rbx]` /
`add eax, 1` / `mov [rbx], eax` — a 32-bit integer load/add/store on 8 bytes
of IEEE 754 double precision bits.

This is a **codegen-only stage**.  No tokenizer, parser, or AST changes are
needed.

---

## Root-cause analysis

`codegen_inc_dec_general` handles `++`/`--` on any general lvalue that is not
a bare identifier (`*p`, `a[i]`, `s.field`, `p->field`).  The function has
three sequential steps: compute the lvalue address, determine the element size
and kind, then load/adjust/store.

### Bug 1 — Wrong size for `double` fields

After resolving the address, the function determines `kind` and `full` from the
returned `StructField`:

```c
} else if (operand->type == AST_MEMBER_ACCESS) {
    StructField *f = emit_member_addr(cg, operand);
    kind = f->kind;
    full = f->full_type;
    sz = full ? type_size(full) : 0;
```

For a `double` field, `f->kind == TYPE_DOUBLE` and `f->full_type == NULL`
(scalars have no full_type).  Therefore `sz = 0`.  The fallback switch then
selects the wrong size:

```c
    if (sz == 0) {
        switch (kind) {
        case TYPE_CHAR:  sz = 1; break;
        case TYPE_SHORT: sz = 2; break;
        case TYPE_LONG_LONG:
        case TYPE_UNSIGNED_LONG_LONG:
        case TYPE_LONG:
        case TYPE_POINTER: sz = 8; break;
        default: sz = 4; break;     /* TYPE_DOUBLE and TYPE_FLOAT both land here */
        }
    }
```

`TYPE_DOUBLE` falls through to `default: sz = 4`.  The subsequent load emits
`mov eax, [rbx]` (4 bytes) instead of the 8-byte load a double requires.
`TYPE_FLOAT` accidentally gets `sz = 4` (correct byte count for float), but
the wrong instructions are still emitted — see Bug 2.

### Bug 2 — Integer instructions used for FP fields

After the size computation, the load/adjust/store uses integer instructions
regardless of `kind`:

```c
    /* rbx = &operand */
    fprintf(cg->output, "    mov rbx, rax\n");

    switch (sz) {
    case 8: fprintf(cg->output, "    mov rax, [rbx]\n"); break;
    case 4:
    default: fprintf(cg->output, "    mov eax, [rbx]\n"); break;
    }

    if (sz == 8) {
        if (is_inc) fprintf(cg->output, "    add rax, %d\n", step);
        else        fprintf(cg->output, "    sub rax, %d\n", step);
    } else {
        if (is_inc) fprintf(cg->output, "    add eax, %d\n", step);
        else        fprintf(cg->output, "    sub eax, %d\n", step);
    }

    switch (sz) {
    case 8: fprintf(cg->output, "    mov [rbx], rax\n"); break;
    case 4:
    default: fprintf(cg->output, "    mov [rbx], eax\n"); break;
    }
```

Even for a `float` field (where `sz = 4` is numerically correct), adding 1 to
the IEEE 754 bit pattern via `add eax, 1` produces nonsense.

---

## Design

The fix adds an FP early-return path at the top of the load/adjust/store
block — immediately after the address is in `rbx` — that handles
`TYPE_DOUBLE` and `TYPE_FLOAT` via SSE2 instructions.  It uses two new
`.rodata` constants:

| Constant        | Encoding           | Used for                        |
|-----------------|--------------------|---------------------------------|
| `Lfp_one_f64`  | `dq 0x3FF0000000000000` | `addsd`/`subsd` for double fields |
| `Lfp_one_f32`  | `dd 0x3F800000`    | `addss`/`subss` for float fields  |

These follow the same pattern as the existing sign-mask constants
(`Lfp_smask_f32` / `Lfp_smask_f64`) introduced in stage 110.

For **prefix** forms (`++s.x`), the result is the new value — `xmm0` already
holds it after the store.

For **postfix** forms (`s.x++`), the old value must be returned.  The old
value is saved in `xmm1` before the increment, and `xmm1` is moved to `xmm0`
at the end.

`addsd`/`subsd` and `addss`/`subss` take an `m64`/`m32` source; no special
alignment is required on the memory operand (unlike `xorps`/`xorpd` which
need 16-byte alignment).

---

## Task 1 — Add `fp_one_f64_emitted` and `fp_one_f32_emitted` to `CodeGen`

In `include/codegen.h`, after the existing sign-mask flags (~line 165):

**Before**:

```c
    int fp_sign_mask_f32_emitted;
    int fp_sign_mask_f64_emitted;
```

**After**:

```c
    int fp_sign_mask_f32_emitted;
    int fp_sign_mask_f64_emitted;
    /* Stage 120: track whether the FP 1.0 constants for ++/-- on FP struct
     * members have been emitted (Lfp_one_f64 / Lfp_one_f32). */
    int fp_one_f64_emitted;
    int fp_one_f32_emitted;
```

---

## Task 2 — Initialize the new flags in `codegen_init`

In `src/codegen.c`, `codegen_init` (~line 447):

**Before**:

```c
    cg->fp_sign_mask_f32_emitted = 0;
    cg->fp_sign_mask_f64_emitted = 0;
```

**After**:

```c
    cg->fp_sign_mask_f32_emitted = 0;
    cg->fp_sign_mask_f64_emitted = 0;
    cg->fp_one_f64_emitted = 0;
    cg->fp_one_f32_emitted = 0;
```

---

## Task 3 — Add FP path to `codegen_inc_dec_general`

In `src/codegen.c`, `codegen_inc_dec_general` (~line 2358), immediately after
the `mov rbx, rax` that saves the operand address and immediately before the
`switch (sz)` load block:

**Before**:

```c
    /* rbx = &operand (preserved across the load/adjust/store). */
    fprintf(cg->output, "    mov rbx, rax\n");

    /* Load the current value into rax with the element width. */
    switch (sz) {
    case 1: fprintf(cg->output, "    movsx eax, byte [rbx]\n"); break;
    case 2: fprintf(cg->output, "    movsx eax, word [rbx]\n"); break;
    case 8: fprintf(cg->output, "    mov rax, [rbx]\n"); break;
    case 4:
    default: fprintf(cg->output, "    mov eax, [rbx]\n"); break;
    }
```

**After** (insert the FP block between `mov rbx, rax` and the existing switch):

```c
    /* rbx = &operand (preserved across the load/adjust/store). */
    fprintf(cg->output, "    mov rbx, rax\n");

    /* Stage 120: FP struct member ++/-- — use SSE2 instructions. */
    if (kind == TYPE_DOUBLE || kind == TYPE_FLOAT) {
        int use_double = (kind == TYPE_DOUBLE);
        if (use_double) {
            cg->fp_one_f64_emitted = 1;
            if (is_post) {
                fprintf(cg->output, "    movsd xmm1, [rbx]\n");  /* save old */
            }
            fprintf(cg->output, "    movsd xmm0, [rbx]\n");
            if (is_inc)
                fprintf(cg->output, "    addsd xmm0, [rel Lfp_one_f64]\n");
            else
                fprintf(cg->output, "    subsd xmm0, [rel Lfp_one_f64]\n");
            fprintf(cg->output, "    movsd [rbx], xmm0\n");
            if (is_post)
                fprintf(cg->output, "    movsd xmm0, xmm1\n");   /* return old */
        } else {
            cg->fp_one_f32_emitted = 1;
            if (is_post) {
                fprintf(cg->output, "    movss xmm1, [rbx]\n");
            }
            fprintf(cg->output, "    movss xmm0, [rbx]\n");
            if (is_inc)
                fprintf(cg->output, "    addss xmm0, [rel Lfp_one_f32]\n");
            else
                fprintf(cg->output, "    subss xmm0, [rel Lfp_one_f32]\n");
            fprintf(cg->output, "    movss [rbx], xmm0\n");
            if (is_post)
                fprintf(cg->output, "    movss xmm0, xmm1\n");
        }
        node->result_type = kind;
        return;
    }

    /* Load the current value into rax with the element width. */
    switch (sz) {
    case 1: fprintf(cg->output, "    movsx eax, byte [rbx]\n"); break;
    case 2: fprintf(cg->output, "    movsx eax, word [rbx]\n"); break;
    case 8: fprintf(cg->output, "    mov rax, [rbx]\n"); break;
    case 4:
    default: fprintf(cg->output, "    mov eax, [rbx]\n"); break;
    }
```

The `return;` exits `codegen_inc_dec_general` before the integer
load/adjust/store block and before the `result_type = promote_kind(kind)` line
at the bottom; the caller sees `node->result_type == TYPE_DOUBLE` (or
`TYPE_FLOAT`) in `xmm0` — exactly the same convention used by all other FP
expression paths.

---

## Task 4 — Emit `Lfp_one_f64` and `Lfp_one_f32` in `.rodata`

In `src/codegen.c`, the `.rodata` emission function (~line 6470), after the
existing sign-mask block:

**Before** (end of the rodata emitter):

```c
    if (cg->fp_sign_mask_f32_emitted)
        fprintf(cg->output,
                "align 16\n"
                "Lfp_smask_f32: dd 0x80000000, 0, 0, 0\n");
    if (cg->fp_sign_mask_f64_emitted)
        fprintf(cg->output,
                "align 16\n"
                "Lfp_smask_f64: dq 0x8000000000000000, 0\n");
}
```

**After**:

```c
    if (cg->fp_sign_mask_f32_emitted)
        fprintf(cg->output,
                "align 16\n"
                "Lfp_smask_f32: dd 0x80000000, 0, 0, 0\n");
    if (cg->fp_sign_mask_f64_emitted)
        fprintf(cg->output,
                "align 16\n"
                "Lfp_smask_f64: dq 0x8000000000000000, 0\n");
    /* Stage 120: 1.0 constants for FP ++/-- on struct members.
     * addsd/subsd and addss/subss take m64/m32 operands with no alignment
     * requirement, so no align 16 padding is needed. */
    if (cg->fp_one_f64_emitted)
        fprintf(cg->output,
                "Lfp_one_f64: dq 0x3FF0000000000000\n");
    if (cg->fp_one_f32_emitted)
        fprintf(cg->output,
                "Lfp_one_f32: dd 0x3F800000\n");
}
```

---

## Task 5 — `src/version.c`

Bump `VERSION_STAGE` to `"01200000"`.

---

## Implementation order

1. Apply Task 1 (add `fp_one_f64_emitted` / `fp_one_f32_emitted` to header).
2. Apply Task 2 (initialize flags in `codegen_init`).
3. Apply Task 3 (FP path in `codegen_inc_dec_general`).
4. Apply Task 4 (`.rodata` emission of `Lfp_one_f64` / `Lfp_one_f32`).
5. Build (`cmake --build build`).
6. Quick smoke test: compile and run `++v.x` on a `double` field, verify exit 3.
7. Run `test/valid/run_tests.sh` — all existing tests must pass.
8. Add tests (see below); run full suite (`./test/run_all_tests.sh`).
9. Bump `src/version.c`.
10. Self-host: `./build.sh --mode=self-host` — all three passes must pass.
11. Update `docs/self-compilation-report.md`.
12. Commit.
13. Delegate README + milestone + transcript to `haiku-stage-artifact-writer`.

---

## Out of scope — do NOT do these in this stage

- **`++`/`--` on non-member FP lvalues** — bare `double d; ++d;` goes through
  a different code path (the non-general `AST_PREFIX_INC_DEC` branch) which
  loads/stores the variable correctly for scalars.  Only the
  `codegen_inc_dec_general` path (struct members, array elements, dereferences)
  is fixed here.
- **`++`/`--` on FP array elements** — `++arr[i]` where the array element
  is `double` also routes through `codegen_inc_dec_general` via the
  `AST_ARRAY_INDEX` branch.  The `emit_array_index_addr` return type carries
  the element `Type *` with `kind = TYPE_DOUBLE`, and the new FP branch in Task
  3 will fire correctly for this case too.  **This is a bonus fix, not a
  separate stage**.
- **`++`/`--` on FP deref** — `++(*dp)` where `dp` is a `double *` also goes
  through `codegen_inc_dec_general` via the `AST_DEREF` branch.  The `kind`
  field is derived from `base->kind`, so the new FP path fires here too.
  **Also a bonus fix**.
- **Mixed-type compound increment** — no implicit int→float conversion for
  `++` on FP fields; C99 `++` always adds exactly 1.0.

---

## Bootstrap caveats

The compiler's own source does not use `++`/`--` on float or double struct
members.  All FP increment patterns in the compiler source use scalar local
variables (not struct members).  The self-host bootstrap is therefore expected
to pass at C0/C1/C2 without source changes.

---

## Tests

### Valid tests — place in `test/valid/structs/`

#### Prefix `++` on `double` field

**`test/valid/structs/test_struct_fp_prefix_inc_double__3.c`**
```c
struct Vec { double x; double y; };

int main(void) {
    struct Vec v;
    v.x = 2.0;
    ++v.x;
    return (int)v.x;   /* 3 */
}
```
Expected exit: 3

#### Prefix `--` on `double` field

**`test/valid/structs/test_struct_fp_prefix_dec_double__4.c`**
```c
struct Vec { double x; double y; };

int main(void) {
    struct Vec v;
    v.x = 5.0;
    --v.x;
    return (int)v.x;   /* 4 */
}
```
Expected exit: 4

#### Postfix `++` returns old value

**`test/valid/structs/test_struct_fp_postfix_inc_double__2.c`**
```c
struct Counter { double val; };

int main(void) {
    struct Counter c;
    c.val = 2.0;
    int old = (int)(c.val++);   /* returns 2 (old value); c.val becomes 3.0 */
    return old;
}
```
Expected exit: 2

#### Postfix `--` returns old value

**`test/valid/structs/test_struct_fp_postfix_dec_double__7.c`**
```c
struct Counter { double val; };

int main(void) {
    struct Counter c;
    c.val = 8.0;
    int old = (int)(c.val--);   /* returns 8; c.val becomes 7.0 */
    return old;
}
```
Expected exit: 8

#### Arrow `++` on `double` field

**`test/valid/structs/test_struct_fp_arrow_inc_double__6.c`**
```c
struct Node { double score; };

int main(void) {
    struct Node n;
    struct Node *p = &n;
    p->score = 5.0;
    ++p->score;
    return (int)p->score;   /* 6 */
}
```
Expected exit: 6

#### `++` on `float` field

**`test/valid/structs/test_struct_fp_prefix_inc_float__4.c`**
```c
struct Pt { float x; float y; };

int main(void) {
    struct Pt pt;
    pt.x = 3.0f;
    ++pt.x;
    return (int)pt.x;   /* 4 */
}
```
Expected exit: 4

#### `++` in a loop accumulates correctly

**`test/valid/structs/test_struct_fp_inc_loop__10.c`**
```c
struct Acc { double sum; };

int main(void) {
    struct Acc a;
    a.sum = 0.0;
    int i;
    for (i = 0; i < 10; i++)
        ++a.sum;
    return (int)a.sum;   /* 10 */
}
```
Expected exit: 10

---

## Docs (at stage close, after all tests pass)

- **`README.md`** — note that `++`/`--` on FP struct members is now supported;
  update test totals.
- **`docs/self-compilation-report.md`** — add stage-120 self-host result.
- Delegate README + milestone + transcript to `haiku-stage-artifact-writer`.
