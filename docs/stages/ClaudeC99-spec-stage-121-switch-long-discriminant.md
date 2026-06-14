# ClaudeC99 Stage 121 — `switch` on `long` / `unsigned long long` Discriminants

## Goal

Fix the `switch` statement so that `long`, `long long`, and `unsigned long
long` discriminants are compared with 64-bit instructions.  The current
dispatch emits:

```asm
mov eax, [rsp]          ; 32-bit load — truncates the top 32 bits!
cmp eax, <case_value>   ; 32-bit compare
```

This produces wrong results when the discriminant holds a value whose top 32
bits are non-zero (any `long` value outside `[−2³¹, 2³¹−1]`).

`int`, `char`, and `short` discriminants are not affected — their values are
integer-promoted to `int` (32-bit) before evaluation and fit inside `eax`.

This is a **codegen-only stage**.  No tokenizer, parser, or AST changes are
needed.

---

## Root-cause analysis

The `AST_SWITCH_STATEMENT` handler in `codegen_statement` (~line 5940)
evaluates the controlling expression and spills it to the stack:

```c
codegen_expression(cg, node->children[0]);
fprintf(cg->output, "    push rax\n");
cg->push_depth++;
```

The dispatch loop then reloads and compares the discriminant against each case
label:

```c
for (int i = 0; i < (int)ctx->entries.len; i++) {
    SwitchLabel *entry = (SwitchLabel *)vec_get(&ctx->entries, i);
    ASTNode *label_node = entry->node;
    if (label_node->type == AST_CASE_SECTION) {
        fprintf(cg->output, "    mov eax, [rsp]\n");     /* ← 32-bit load */
        fprintf(cg->output, "    cmp eax, %s\n",         /* ← 32-bit compare */
                label_node->children[0]->value);
        fprintf(cg->output, "    je .L_switch_sec_%d\n",
                entry->label);
    }
}
```

`mov eax, [rsp]` loads only the low 32 bits of the 8-byte stack slot.  For a
`long` discriminant with value `10000000000L` (0x00000002_540BE400), the load
gives `eax = 0x540BE400` — the wrong value — so no case label matches.

The fix is to check the controlling expression's `result_type` (set by
`codegen_expression`) and emit `mov rax, [rsp]` / `cmp rax, <val>` when the
discriminant is a 64-bit integer type.

### Why `char`/`short` are unaffected

The C99 integer promotions apply to the controlling expression before the
switch.  A `char` or `short` controlling expression is promoted to `int` during
evaluation in `codegen_expression`, so `result_type` comes back as `TYPE_INT`
and `rax` holds a sign-extended 32-bit value.  Reloading with `mov eax, [rsp]`
captures the full value correctly.

### Case value encoding for 64-bit discriminants

For `cmp rax, <val>`, the x86-64 `CMP r64, imm32` instruction sign-extends a
32-bit immediate to 64 bits.  This covers all case values in the range
`[−2³¹, 2³¹−1]` — which is the vast majority of `long` case values in
practice (error codes, enum values, small constants).

Case values outside that range (e.g., `case 10000000000L:`) cannot be encoded
as `cmp rax, imm32`; NASM would fail to assemble them.  That edge case is out
of scope for this stage (see Out of scope below).

---

## Task 1 — Detect discriminant type and emit 64-bit dispatch

In `codegen_statement`, `AST_SWITCH_STATEMENT` handler, replace the dispatch
loop (~line 5954):

**Before**:

```c
        codegen_expression(cg, node->children[0]);
        fprintf(cg->output, "    push rax\n");
        cg->push_depth++;

        for (int i = 0; i < (int)ctx->entries.len; i++) {
            SwitchLabel *entry = (SwitchLabel *)vec_get(&ctx->entries, i);
            ASTNode *label_node = entry->node;
            if (label_node->type == AST_CASE_SECTION) {
                fprintf(cg->output, "    mov eax, [rsp]\n");
                fprintf(cg->output, "    cmp eax, %s\n",
                        label_node->children[0]->value);
                fprintf(cg->output, "    je .L_switch_sec_%d\n",
                        entry->label);
            }
        }
```

**After**:

```c
        codegen_expression(cg, node->children[0]);
        fprintf(cg->output, "    push rax\n");
        cg->push_depth++;

        /* Stage 121: use 64-bit reload and compare for long/long long
         * discriminants; 32-bit is correct for int/char/short (promoted). */
        TypeKind disc_kind = node->children[0]->result_type;
        int disc_is_64 = (disc_kind == TYPE_LONG ||
                          disc_kind == TYPE_LONG_LONG ||
                          disc_kind == TYPE_UNSIGNED_LONG_LONG);

        for (int i = 0; i < (int)ctx->entries.len; i++) {
            SwitchLabel *entry = (SwitchLabel *)vec_get(&ctx->entries, i);
            ASTNode *label_node = entry->node;
            if (label_node->type == AST_CASE_SECTION) {
                if (disc_is_64) {
                    fprintf(cg->output, "    mov rax, [rsp]\n");
                    fprintf(cg->output, "    cmp rax, %s\n",
                            label_node->children[0]->value);
                } else {
                    fprintf(cg->output, "    mov eax, [rsp]\n");
                    fprintf(cg->output, "    cmp eax, %s\n",
                            label_node->children[0]->value);
                }
                fprintf(cg->output, "    je .L_switch_sec_%d\n",
                        entry->label);
            }
        }
```

The `disc_kind` is captured immediately after `codegen_expression` so
`result_type` is fresh (it is set by `codegen_expression` as a side-effect).

---

## Task 2 — `src/version.c`

Bump `VERSION_STAGE` to `"01210000"`.

---

## Implementation order

1. Apply Task 1 (64-bit discriminant detection and dispatch).
2. Build (`cmake --build build`).
3. Quick smoke test: `long l = 3000000000L; switch(l) { case 3000000000L: return 42; }` — verify exit 42.
4. Run `test/valid/run_tests.sh` — all existing tests must pass.
5. Add tests (see below); run full suite (`./test/run_all_tests.sh`).
6. Bump `src/version.c`.
7. Self-host: `./build.sh --mode=self-host` — all three passes must pass.
8. Update `docs/self-compilation-report.md`.
9. Commit.
10. Delegate README + milestone + transcript to `haiku-stage-artifact-writer`.

---

## Out of scope — do NOT do these in this stage

- **`case` values outside `[−2³¹, 2³¹−1]` for `long` discriminants** — `cmp
  rax, imm32` cannot encode a 64-bit immediate directly.  Assembling
  `case 10000000000L:` against a `long` discriminant requires emitting
  `mov rcx, 10000000000; cmp rax, rcx`.  This edge case is rare and is deferred
  to a future diagnostic or extension stage.
- **`switch` on `TYPE_POINTER`** — comparing a pointer discriminant against
  case labels is undefined behavior in C99 and is not supported.
- **FP discriminants** — `switch(f)` where `f` is `float` or `double` is a
  constraint violation in C99 and must be rejected.  The existing code reaches a
  `compile_error` before the dispatch loop; no change needed here.
- **Jump tables** — the current linear compare-and-branch dispatch is
  intentionally kept.  Generating a jump table for dense case ranges is an
  optimization out of scope for this stage.

---

## Bootstrap caveats

The compiler's own source uses `switch` extensively on `int`-typed
discriminants (TypeKind enums, token types, etc.).  None of the existing
switches use a `long` or `long long` discriminant.  The `disc_is_64` flag will
be `0` for every switch in the compiler, so C0→C1→C2 will produce identical
code for existing switch statements.  The self-host bootstrap is expected to
pass without source changes.

---

## Tests

### Valid tests — place in `test/valid/statements/`

#### `switch` on `long` — small value, standard case

**`test/valid/statements/test_switch_long_small__3.c`**
```c
int main(void) {
    long x = 3L;
    switch (x) {
    case 1L: return 10;
    case 2L: return 20;
    case 3L: return 3;
    default: return 99;
    }
}
```
Expected exit: 3

#### `switch` on `long` — default arm taken

**`test/valid/statements/test_switch_long_default__42.c`**
```c
int main(void) {
    long x = 99L;
    switch (x) {
    case 1L:  return 1;
    case 2L:  return 2;
    default:  return 42;
    }
}
```
Expected exit: 42

#### `switch` on `long long`

**`test/valid/statements/test_switch_llong__7.c`**
```c
int main(void) {
    long long n = 7LL;
    switch (n) {
    case 5LL: return 5;
    case 7LL: return 7;
    case 9LL: return 9;
    default:  return 0;
    }
}
```
Expected exit: 7

#### `switch` on `unsigned long long`

**`test/valid/statements/test_switch_ullong__4.c`**
```c
int main(void) {
    unsigned long long u = 4ULL;
    switch (u) {
    case 1ULL: return 1;
    case 4ULL: return 4;
    case 8ULL: return 8;
    default:   return 0;
    }
}
```
Expected exit: 4

#### `switch` on `char` — regression (must still use 32-bit compare)

**`test/valid/statements/test_switch_char_regression__65.c`**
```c
int main(void) {
    char c = 'A';   /* 65 */
    switch (c) {
    case 'A': return 65;
    case 'B': return 66;
    default:  return 0;
    }
}
```
Expected exit: 65

#### `switch` on `int` — regression (unchanged behavior)

**`test/valid/statements/test_switch_int_regression__5.c`**
```c
int main(void) {
    int n = 5;
    switch (n) {
    case 3:  return 3;
    case 5:  return 5;
    case 7:  return 7;
    default: return 0;
    }
}
```
Expected exit: 5

---

## Docs (at stage close, after all tests pass)

- **`README.md`** — update to note `switch` now works correctly on all integer
  types including `long` and `long long`; update test totals.
- **`docs/self-compilation-report.md`** — add stage-121 self-host result.
- Delegate README + milestone + transcript to `haiku-stage-artifact-writer`.
