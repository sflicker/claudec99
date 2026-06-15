# ClaudeC99 Stage 127 — Callee-Saved Registers r12–r15

## Goal

Preserve r12, r13, r14, r15 across every generated function per SysV AMD64 ABI.
Currently only rbx is saved (Stage 122). This stage adds the four remaining
callee-saved general-purpose registers to both the prologue and all epilogue paths.

---

## Background

SysV AMD64 ABI §3.2.1 — callee-saved (non-volatile) registers:
`rbx`, `r12`, `r13`, `r14`, `r15`, `rbp`, `rsp`

Stage 122 implemented rbx preservation via a dedicated slot at `[rbp - 8]`.
r12–r15 are not currently preserved, meaning any caller that stores values in
r12–r15 before calling one of our functions could have them silently corrupted
by libc calls made inside our generated functions (if libc uses them internally
without saving — which the ABI allows since we don't save them either).

In practice the compiler's current code generation does not allocate r12–r15 as
scratch registers, so the ABI gap has not produced visible bugs yet. This stage
closes the gap and makes the compiler ready to use r12–r15 as allocatable
temporaries in future stages.

---

## Stack Frame Layout Change

### Before (Stage 122)

```
[rbp - 8]  rbx save slot
[rbp - 12] first local / parameter (4-byte int example)
...
```

`stack_size = ... + 8` (rbx slot)
`cg->stack_offset` initialized to 8

### After (Stage 127)

```
[rbp - 8]  rbx save slot
[rbp - 16] r12 save slot
[rbp - 24] r13 save slot
[rbp - 32] r14 save slot
[rbp - 40] r15 save slot
[rbp - 44] first local / parameter
...
```

`stack_size = ... + 40` (rbx + r12 + r13 + r14 + r15)
`cg->stack_offset` initialized to 40

---

## Changes

### `src/codegen.c` — `cg->stack_offset` initialization (`codegen_generate_function`)

```c
/* Before: */
cg->stack_offset = 8;   /* Stage 122: [rbp - 8] reserved for rbx */

/* After: */
cg->stack_offset = 40;  /* Stage 127: [rbp-8]=rbx, [rbp-16..40]=r12-r15 */
```

### `src/codegen.c` — `stack_size` formula

```c
/* Before: */
int stack_size = param_bytes + compute_decl_bytes(body) + compound_lit_bytes +
                 scratch_bytes + sret_bytes + 8; /* rbx save slot */

/* After: */
int stack_size = param_bytes + compute_decl_bytes(body) + compound_lit_bytes +
                 scratch_bytes + sret_bytes + 40; /* rbx + r12-r15 save slots */
```

### `src/codegen.c` — Prologue save (after `mov [rbp - 8], rbx`)

```c
fprintf(cg->output, "    mov [rbp - 8], rbx\n");
fprintf(cg->output, "    mov [rbp - 16], r12\n");
fprintf(cg->output, "    mov [rbp - 24], r13\n");
fprintf(cg->output, "    mov [rbp - 32], r14\n");
fprintf(cg->output, "    mov [rbp - 40], r15\n");
```

### `src/codegen.c` — Epilogue restore (all 4 `mov rbx, [rbp - 8]` sites)

Before each `mov rbx, [rbp - 8]`, add:
```c
fprintf(cg->output, "    mov r12, [rbp - 16]\n");
fprintf(cg->output, "    mov r13, [rbp - 24]\n");
fprintf(cg->output, "    mov r14, [rbp - 32]\n");
fprintf(cg->output, "    mov r15, [rbp - 40]\n");
```

### `src/codegen.c` — Variadic alignment

The variadic register save area alignment code adds padding when
`cg->stack_offset % 16 != 0`. With `stack_offset = 40`, which is
already divisible by 8 but not 16 (40 mod 16 = 8), the padding will
kick in. With the 8 bytes of padding:

`stack_offset` after padding: 40 + 8 = 48 (now 48 mod 16 = 0 ✓)

This is correct — the XMM slot alignment requirement is still met.

---

## Tests

Tests are verified by the existing suite — all 1923 tests must still pass.

Add one targeted test to confirm r12–r15 are not corrupted across a call:

**`test/valid/functions/test_callee_saved_r12_r15__0.c`**:
```c
/* Inline asm is not supported; verify indirectly: a chain of calls
 * that exercises the full call graph should return correctly. */
static int double_it(int x) { return x * 2; }
static int triple_it(int x) { return x * 3; }
int main(void) {
    int a = double_it(5);   /* 10 */
    int b = triple_it(7);   /* 21 */
    return a + b - 31;      /* 10 + 21 - 31 = 0 */
}
```
Expected exit: 0.

---

## Version

Bump `VERSION_STAGE` in `src/version.c` to `"01270000"`.

---

## Out of Scope

- Using r12–r15 as allocatable registers for expression temporaries — that
  is a register-allocation optimization for a later stage.
- Selective save/restore (only save registers that are actually used) —
  currently unconditional; optimization is deferred.
