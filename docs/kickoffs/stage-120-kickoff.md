# Stage 120 Kickoff — FP Increment/Decrement on Struct Members

## Summary

Stage 120 fixes prefix and postfix `++`/`--` operators on `float` and `double` struct fields accessed via dot (`.`) and arrow (`->`) operators. This is a **codegen-only stage** — no tokenizer, parser, or AST changes required.

The bugs are in `codegen_inc_dec_general`:
1. **Bug 1**: `TYPE_DOUBLE` falls through to `default: sz = 4` in the fallback size switch (should be 8).
2. **Bug 2**: Integer instructions (`add eax/rax, 1`) are used regardless of FP type, producing incorrect bit patterns.

The fix adds an FP early-return path using SSE2 instructions and two new `.rodata` constants (`Lfp_one_f64` and `Lfp_one_f32`).

---

## Required Tokenizer Changes

None.

---

## Required Parser Changes

None.

---

## Required AST Changes

None.

---

## Required Code Generation Changes

### 1. Add FP one-constant emission flags to `include/codegen.h`

After the existing sign-mask flags (~line 165), add:

```c
    int fp_one_f64_emitted;
    int fp_one_f32_emitted;
```

These track whether `Lfp_one_f64` and `Lfp_one_f32` have been emitted to `.rodata`.

### 2. Initialize new flags in `src/codegen.c` `codegen_init`

In `codegen_init` (~line 447), after initializing the sign-mask flags:

```c
    cg->fp_one_f64_emitted = 0;
    cg->fp_one_f32_emitted = 0;
```

### 3. Add FP path to `src/codegen.c` `codegen_inc_dec_general`

In `codegen_inc_dec_general` (~line 2358), immediately after `mov rbx, rax` and before the `switch (sz)` load block, insert an FP early-return:

```c
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
```

This path returns early with the FP value in `xmm0`, avoiding the integer load/adjust/store block.

### 4. Emit `Lfp_one_f64` and `Lfp_one_f32` in `.rodata`

In the `.rodata` emission function (~line 6470), after the sign-mask block, add:

```c
    /* Stage 120: 1.0 constants for FP ++/-- on struct members.
     * addsd/subsd and addss/subss take m64/m32 operands with no alignment
     * requirement, so no align 16 padding is needed. */
    if (cg->fp_one_f64_emitted)
        fprintf(cg->output,
                "Lfp_one_f64: dq 0x3FF0000000000000\n");
    if (cg->fp_one_f32_emitted)
        fprintf(cg->output,
                "Lfp_one_f32: dd 0x3F800000\n");
```

### 5. Bump version in `src/version.c`

Change `VERSION_STAGE` to `"01200000"`.

---

## Test Plan

Seven new tests in `test/valid/structs/`:

1. **`test_struct_fp_prefix_inc_double__3.c`** — `++v.x` on double field → exit 3
2. **`test_struct_fp_prefix_dec_double__4.c`** — `--v.x` on double field → exit 4
3. **`test_struct_fp_postfix_inc_double__2.c`** — `c.val++` returns old value → exit 2
4. **`test_struct_fp_postfix_dec_double__7.c`** — `c.val--` returns old value → exit 8
5. **`test_struct_fp_arrow_inc_double__6.c`** — `++p->score` via arrow → exit 6
6. **`test_struct_fp_prefix_inc_float__4.c`** — `++pt.x` on float field → exit 4
7. **`test_struct_fp_inc_loop__10.c`** — loop with `++a.sum` accumulates → exit 10

All test sources are provided in the spec.

---

## Implementation Checklist

- [ ] Task 1: Add `fp_one_f64_emitted` / `fp_one_f32_emitted` to `include/codegen.h`
- [ ] Task 2: Initialize flags in `codegen_init`
- [ ] Task 3: Add FP early-return path in `codegen_inc_dec_general`
- [ ] Task 4: Emit constants in `.rodata` emitter
- [ ] Build and smoke test (`++v.x` on double → exit 3)
- [ ] Run existing test suite (`test/valid/run_tests.sh`)
- [ ] Add seven new test files
- [ ] Run full test suite (`test/run_all_tests.sh`)
- [ ] Bump `src/version.c`
- [ ] Self-host bootstrap (`./build.sh --mode=self-host`)
- [ ] Update `docs/self-compilation-report.md`
- [ ] Commit with co-author trailer
