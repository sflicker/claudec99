# ClaudeC99 Stage 122 — Callee-Saved `rbx` Preservation

## Goal

Fix the generated code to preserve the `rbx` register across function calls, as
required by the SysV AMD64 ABI (§3.2.1: `rbx`, `r12`–`r15` are callee-saved).

The compiler uses `rbx` extensively as a scratch register for address
arithmetic (array indexing, struct member assignment, general lvalue
increment/decrement).  The function prologue never saves `rbx`; the epilogue
never restores it.  This violates the ABI and causes silent data corruption
when:

- Our generated code is used as a **callback** by external library code (e.g.,
  a `qsort` comparator, a `bsearch` comparator, a signal handler) — the
  external code saves `rbx` before the call expecting it to be preserved, but
  our function clobbers it through its push/pop scratch pattern.
- An external library that uses `rbx` is called from a context where our code
  has pushed a value and popped it into `rbx` (the normal use pattern) — this
  is safe because our push/pop pairs are balanced within each expression, so
  `rbx` is never live-across external calls in our code.  The bug direction is
  external-caller → our-callee.

This is a **codegen-only stage**.  No tokenizer, parser, or AST changes are
needed.

---

## Root-cause analysis

### How `rbx` is used

The compiler's address-computation helpers use a push/pop pair to keep two
values in-flight during index-scaling or member-offset addition:

```asm
; Example: computing arr[i].field address
push rax              ; save base address
; compute index * stride → rax
pop rbx               ; base address → rbx
add rax, rbx          ; base + index*stride
```

This pattern appears in `emit_array_index_addr`, `emit_member_addr`,
`emit_arrow_addr`, and their callers.  The push/pop is always balanced within a
single expression subtree — `rbx` is never live across a function call that
originates from our generated code.

However, when **external code** calls one of our functions and that function
happens to clobber `rbx` during its own internal computations, the external
caller's saved value is lost.  glibc's `qsort`, for example, uses `rbx` as a
loop index across the calls to the comparator function; if our comparator
clobbers `rbx`, the sort produces wrong results or crashes.

### Current prologue (missing rbx save)

```c
fprintf(cg->output, "    push rbp\n");
fprintf(cg->output, "    mov rbp, rsp\n");
if (stack_size > 0) {
    fprintf(cg->output, "    sub rsp, %d\n", stack_size);
}
// ← NO save of rbx
```

### Current epilogue (missing rbx restore)

```c
if (cg->has_frame) {
    fprintf(cg->output, "    mov rsp, rbp\n");
    fprintf(cg->output, "    pop rbp\n");
}
fprintf(cg->output, "    ret\n");
// ← NO restore of rbx
```

---

## Design

The fix saves `rbx` in a dedicated 8-byte slot at `[rbp - 8]` — the first 8
bytes of every function's stack frame — and restores it before every `ret`.

This approach (save-via-`mov`, not via `push`) integrates with the existing
`mov rsp, rbp; pop rbp; ret` epilogue sequence without requiring changes to the
epilogue structure.  Only the prologue grows by one store (`mov [rbp - 8], rbx`)
and the epilogue by one load (`mov rbx, [rbp - 8]`).

### Frame layout changes

Before stage 122 (frame starts immediately with locals):
```
[rbp]       — saved caller rbp (pushed by push rbp)
[rbp -  4]  — first 4-byte local variable
[rbp -  8]  — first 8-byte local (or second 4-byte)
...
```

After stage 122 (slot 0 is reserved for rbx):
```
[rbp]       — saved caller rbp (pushed by push rbp)
[rbp -  8]  — RESERVED: saved rbx
[rbp - 12]  — first 4-byte local variable
[rbp - 16]  — first 8-byte local (or second 4-byte)
...
```

### Stack alignment

After `push rbp; mov rbp, rsp; sub rsp, N`:
- RSP = call_rsp − 8 (return addr) − 8 (push rbp) − N = call_rsp − 16 − N

For RSP to be 16-byte aligned at call sites within the function, N must be ≡ 0
mod 16 (since call_rsp ≡ 0 mod 16 and −16 ≡ 0 mod 16).  The existing prologue
already ensures this by rounding `stack_size` up to the next multiple of 16.
The new rbx slot is part of `stack_size` (added as +8 to the precomputed total,
then rounded to 16), so no alignment formula change is needed.

### Variadic alignment

Variadic functions require the XMM register save area (16 bytes per register)
to be 16-byte aligned.  The current code allocates the save area by
incrementing `cg->stack_offset`:

```c
cg->stack_offset += 176;           /* 6 GP × 8 + 8 XMM × 16 = 48 + 128 */
cg->variadic_reg_save_offset = cg->stack_offset;   /* must be ≡ 0 mod 16 */
```

`cg->stack_offset` starts at 0 currently, so it goes from 0 → 176 (≡ 0 mod
16).  After stage 122, `cg->stack_offset` starts at 8 (for the rbx slot), so
it would go from 8 → 184 (≡ 8 mod 16) — misaligned for `movaps`.

Fix: round `cg->stack_offset` to the next multiple of 16 before adding 176.
With starting offset 8, this pads to 16 (+8 bytes) then adds 176 = 192 (≡ 0
mod 16 ✓).  The precomputed `stack_size` must include the same +8 pad for
variadic functions.

---

## Task 1 — Pre-reserve the rbx slot in `stack_size`

In `codegen_function` (~line 6120), change the `stack_size` computation:

**Before**:

```c
        int stack_size = param_bytes + compute_decl_bytes(body) + compound_lit_bytes +
                         scratch_bytes + sret_bytes;
        if (node->is_variadic)
            stack_size += 176;
        if (stack_size % 16 != 0)
            stack_size = (stack_size + 15) & ~15;
```

**After**:

```c
        int stack_size = param_bytes + compute_decl_bytes(body) + compound_lit_bytes +
                         scratch_bytes + sret_bytes + 8; /* Stage 122: rbx save slot */
        if (node->is_variadic) {
            /* Stage 122: the rbx slot shifts the save-area base by 8; add 8 bytes
             * of alignment padding so the variadic register save area starts at an
             * offset ≡ 0 mod 16 (required for movaps on the XMM slots). */
            stack_size += 8;
            stack_size += 176;
        }
        if (stack_size % 16 != 0)
            stack_size = (stack_size + 15) & ~15;
```

---

## Task 2 — Start `cg->stack_offset` at 8

In `codegen_function` (~line 6073), the per-function reset:

**Before**:

```c
        cg->locals.len = 0;
        cg->stack_offset = 0;
        cg->scope_start = 0;
```

**After**:

```c
        cg->locals.len = 0;
        cg->stack_offset = 8;   /* Stage 122: [rbp - 8] reserved for rbx */
        cg->scope_start = 0;
```

---

## Task 3 — Align variadic save area offset

In `codegen_function`, inside `if (node->is_variadic)` (~line 6158), before
the `cg->stack_offset += 176` line:

**Before**:

```c
        if (node->is_variadic) {
            int named_gp = 0, named_xmm = 0;
            int i;
            for (i = 0; i < num_params; i++) { ... }
            cg->stack_offset += 176;
            cg->variadic_reg_save_offset = cg->stack_offset;
```

**After**:

```c
        if (node->is_variadic) {
            int named_gp = 0, named_xmm = 0;
            int i;
            for (i = 0; i < num_params; i++) { ... }
            /* Stage 122: round to 16 before adding the save area so
             * XMM slots (16-byte movaps) remain 16-byte aligned.
             * With stack_offset = 8 (rbx slot), this adds 8 bytes of pad. */
            if (cg->stack_offset % 16 != 0)
                cg->stack_offset = (cg->stack_offset + 15) & ~15;
            cg->stack_offset += 176;
            cg->variadic_reg_save_offset = cg->stack_offset;
```

---

## Task 4 — Save `rbx` after the prologue `sub rsp`

In `codegen_function`, immediately after the `if (stack_size > 0) { sub rsp
...}` block (~line 6143):

**Before**:

```c
        if (stack_size > 0) {
            fprintf(cg->output, "    sub rsp, %d\n", stack_size);
        }

        /* Stage 75-04/112: variadic function register save area. */
        if (node->is_variadic) {
```

**After**:

```c
        if (stack_size > 0) {
            fprintf(cg->output, "    sub rsp, %d\n", stack_size);
        }
        /* Stage 122: save rbx in the dedicated slot ([rbp - 8]) reserved
         * in Task 1/2.  stack_size ≥ 8 always after Task 1. */
        fprintf(cg->output, "    mov [rbp - 8], rbx\n");

        /* Stage 75-04/112: variadic function register save area. */
        if (node->is_variadic) {
```

---

## Task 5 — Restore `rbx` in all epilogue paths

There are four places in `src/codegen.c` where `pop rbp; ret` is emitted.
In each one, add `mov rbx, [rbp - 8]` immediately before the `mov rsp, rbp`
line.

### Epilogue A — bare `return;` from a void function (~line 5709)

**Before**:

```c
            if (cg->has_frame) {
                fprintf(cg->output, "    mov rsp, rbp\n");
                fprintf(cg->output, "    pop rbp\n");
            }
            fprintf(cg->output, "    ret\n");
```

**After**:

```c
            if (cg->has_frame) {
                fprintf(cg->output, "    mov rbx, [rbp - 8]\n");
                fprintf(cg->output, "    mov rsp, rbp\n");
                fprintf(cg->output, "    pop rbp\n");
            }
            fprintf(cg->output, "    ret\n");
```

### Epilogue B — struct-by-value return (~line 5749)

**Before**:

```c
                if (cg->has_frame) {
                    fprintf(cg->output, "    mov rsp, rbp\n");
                    fprintf(cg->output, "    pop rbp\n");
                }
                fprintf(cg->output, "    ret\n");
                return;
```

**After**:

```c
                if (cg->has_frame) {
                    fprintf(cg->output, "    mov rbx, [rbp - 8]\n");
                    fprintf(cg->output, "    mov rsp, rbp\n");
                    fprintf(cg->output, "    pop rbp\n");
                }
                fprintf(cg->output, "    ret\n");
                return;
```

### Epilogue C — normal `return <expr>;` (~line 5808)

**Before**:

```c
            if (cg->has_frame) {
                fprintf(cg->output, "    mov rsp, rbp\n");
                fprintf(cg->output, "    pop rbp\n");
            }
            fprintf(cg->output, "    ret\n");
```

**After**:

```c
            if (cg->has_frame) {
                fprintf(cg->output, "    mov rbx, [rbp - 8]\n");
                fprintf(cg->output, "    mov rsp, rbp\n");
                fprintf(cg->output, "    pop rbp\n");
            }
            fprintf(cg->output, "    ret\n");
```

### Epilogue D — implicit fall-off-end for void functions (~line 6409)

**Before**:

```c
        if (node->decl_type == TYPE_VOID) {
            fprintf(cg->output, "    mov rsp, rbp\n");
            fprintf(cg->output, "    pop rbp\n");
            fprintf(cg->output, "    ret\n");
        }
```

**After**:

```c
        if (node->decl_type == TYPE_VOID) {
            fprintf(cg->output, "    mov rbx, [rbp - 8]\n");
            fprintf(cg->output, "    mov rsp, rbp\n");
            fprintf(cg->output, "    pop rbp\n");
            fprintf(cg->output, "    ret\n");
        }
```

---

## Task 6 — `src/version.c`

Bump `VERSION_STAGE` to `"01220000"`.

---

## Implementation order

1. Apply Task 1 (stack_size pre-computation adds rbx slot and variadic pad).
2. Apply Task 2 (`cg->stack_offset` starts at 8).
3. Apply Task 3 (variadic alignment round before +176).
4. Apply Task 4 (save rbx in prologue).
5. Apply Task 5 A–D (restore rbx in all epilogues).
6. Build (`cmake --build build`).
7. Quick smoke: compile and run the existing `test_stdlib_qsort__0.c` — must exit 0.
8. Run `test/valid/run_tests.sh` — all existing tests must pass.
9. Add tests (see below); run full suite (`./test/run_all_tests.sh`).
10. Bump `src/version.c`.
11. Self-host: `./build.sh --mode=self-host` — all three passes must pass.
    **Important**: C1 will generate `mov [rbp - 8], rbx` / `mov rbx, [rbp - 8]` in every function, shifting all local-variable offsets up by 8.  C0→C1 will produce different assembly for every function (new instructions + shifted offsets), but the test suite validates behavior, not assembly identity.  C1→C2 must produce C2 that passes all tests.
12. Update `docs/self-compilation-report.md`.
13. Commit.
14. Delegate README + milestone + transcript to `haiku-stage-artifact-writer`.

---

## Out of scope — do NOT do these in this stage

- **`r12`–`r15` preservation** — the compiler's codegen does not use `r12`
  through `r15` as scratch registers, so they are already implicitly
  preserved (never clobbered).  Only `rbx` requires an explicit save/restore.
- **Callee-saved XMM registers** — the SysV AMD64 ABI does not require callee
  saving of any XMM registers.  `xmm0`–`xmm15` are all caller-saved.
- **`rbp` preservation** — `rbp` is a callee-saved register but is already
  correctly saved (`push rbp`) and restored (`pop rbp`) in every function
  prologue/epilogue.
- **Optimization: omit rbx save when unused** — detecting whether a specific
  function body actually clobbers `rbx` would require a complete liveness
  analysis pass.  Unconditional save/restore is the safe and simple choice.
- **`r10`/`r11` scratch** — the compiler also uses `r10` and `r11` in the
  struct-by-value calling convention code.  These are caller-saved registers
  and require no save/restore.

---

## Bootstrap caveats

This stage changes the assembly output of **every compiled function**: the
prologue gains one `mov [rbp - 8], rbx` store and each epilogue gains one
`mov rbx, [rbp - 8]` load.  Additionally, all local variable offsets increase
by 8 because `cg->stack_offset` now starts at 8 instead of 0.

The consequence for self-hosting:

- **C0 → C1**: C0 (GCC-compiled, no rbx save) compiles the stage-122 source
  into C1 which DOES save/restore rbx.  C1's assembly differs from C0's for
  every function.  This is expected and correct.
- **C1 → C2**: C1 compiles the same source into C2.  C2's assembly must match
  C1's instruction-for-instruction (both apply the same rbx-save scheme) — this
  is the key self-host invariant.
- **Test suite**: all 1879 existing tests must pass at C0, C1, and C2.  The
  behavioral contract (correct outputs for all test programs) is unchanged; only
  the generated code structure changes.

The print-asm tests (`test/print_asm/`) compare expected assembly text.  After
stage 122, every expected-output file in that suite must be regenerated to
include `mov [rbp - 8], rbx` in prologues and `mov rbx, [rbp - 8]` in
epilogues, plus adjusted local-variable offsets.  Regenerate the expected
outputs using C1 after it passes the full valid/invalid test suites.

---

## Tests

### Valid tests — place in `test/valid/pointers/`

#### `qsort` comparator that uses struct member access

The comparator function uses struct member access (which triggers the push/pop
rbx pattern), exercising the ABI fix when called by glibc's `qsort` which is
known to use `rbx` internally.

**`test/valid/pointers/test_qsort_struct_cmp__0.c`**
```c
#include <stdlib.h>
#include <string.h>

struct Item { int key; int val; };

static int item_cmp(const void *a, const void *b) {
    const struct Item *ia = (const struct Item *)a;
    const struct Item *ib = (const struct Item *)b;
    return ia->key - ib->key;   /* uses arrow access (rbx scratch) */
}

int main(void) {
    struct Item items[4];
    items[0].key = 30; items[0].val = 3;
    items[1].key = 10; items[1].val = 1;
    items[2].key = 40; items[2].val = 4;
    items[3].key = 20; items[3].val = 2;

    qsort(items, 4, sizeof(struct Item), item_cmp);

    /* After sort: keys should be 10, 20, 30, 40 */
    if (items[0].key != 10) return 1;
    if (items[1].key != 20) return 2;
    if (items[2].key != 30) return 3;
    if (items[3].key != 40) return 4;
    return 0;
}
```
Expected exit: 0

#### `qsort` on integer array — regression (existing test still passes)

The existing `test/valid/stdlib/test_stdlib_qsort__0.c` uses a comparator that
dereferences pointer arguments (also uses rbx scratch).  It must continue to
pass.

#### Array-index address computation in callback context

**`test/valid/pointers/test_array_index_in_callback__0.c`**
```c
#include <stdlib.h>

static int arr_cmp(const void *a, const void *b) {
    /* Uses array subscript on the cast pointer — triggers emit_array_index_addr
     * which uses the push/pop rbx pattern. */
    const int *p = (const int *)a;
    const int *q = (const int *)b;
    int x = p[0];    /* subscript: uses rbx scratch */
    int y = q[0];
    return x - y;
}

int main(void) {
    int data[5] = {5, 2, 8, 1, 9};
    qsort(data, 5, sizeof(int), arr_cmp);
    return (data[0] == 1 && data[4] == 9) ? 0 : 1;
}
```
Expected exit: 0

---

## Docs (at stage close, after all tests pass)

- **`README.md`** — note that the ABI callee-saved `rbx` is now preserved;
  update test totals.
- **`docs/self-compilation-report.md`** — add stage-122 self-host result, noting
  that print-asm expected files were regenerated.
- Delegate README + milestone + transcript to `haiku-stage-artifact-writer`.

### Note on print-asm test regeneration

After stage 122, every function's assembly output has changed (new rbx save/
restore instructions, shifted local-variable offsets).  The `test/print_asm/`
expected output files must be regenerated:

1. After C1 passes all valid/invalid tests, run each `test/print_asm/*.c` file
   through C1 and capture the output as the new expected file.
2. Inspect the diffs to confirm the only changes are the new
   `mov [rbp - 8], rbx` / `mov rbx, [rbp - 8]` lines and adjusted offsets.
3. Commit the updated expected files together with the source changes.
