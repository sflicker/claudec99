# Stage 121 Kickoff — `switch` on `long` / `unsigned long long` Discriminants

## Summary

Stage 121 fixes the `switch` statement so that `long`, `long long`, and `unsigned long long` discriminants are compared with 64-bit instructions. This is a **codegen-only stage** — no tokenizer, parser, or AST changes required.

The bug is in the dispatch loop of the `AST_SWITCH_STATEMENT` handler in `src/codegen.c`. Currently it emits `mov eax, [rsp]` (32-bit load), which truncates the top 32 bits of 64-bit discriminant values. The fix detects the discriminant's `result_type` and emits `mov rax, [rsp]` / `cmp rax, <val>` for 64-bit types.

Integer promotions ensure that `char` and `short` discriminants are promoted to `int` (32-bit) before evaluation, so they are unaffected.

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

### 1. Fix `AST_SWITCH_STATEMENT` dispatch in `src/codegen.c`

In the `AST_SWITCH_STATEMENT` handler (~line 5975), replace the dispatch loop to detect 64-bit discriminants:

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

The `disc_kind` is captured immediately after `codegen_expression` to ensure `result_type` is fresh.

### 2. Bump version in `src/version.c`

Change `VERSION_STAGE` to `"01210000"`.

---

## Test Plan

Six new tests in `test/valid/statements/`:

1. **`test_switch_long_small__3.c`** — long discriminant, small value, case matches → exit 3
2. **`test_switch_long_default__42.c`** — long discriminant, default arm taken → exit 42
3. **`test_switch_llong__7.c`** — long long discriminant → exit 7
4. **`test_switch_ullong__4.c`** — unsigned long long discriminant → exit 4
5. **`test_switch_char_regression__65.c`** — char discriminant (32-bit path regression) → exit 65
6. **`test_switch_int_regression__5.c`** — int discriminant (unchanged behavior) → exit 5

All test sources are provided in the spec.

---

## Implementation Checklist

- [ ] Task 1: Apply dispatch loop fix in `src/codegen.c`
- [ ] Build and smoke test (long discriminant 3000000000L → exit 42)
- [ ] Run existing test suite (`test/valid/run_tests.sh`)
- [ ] Add six new test files
- [ ] Run full test suite (`test/run_all_tests.sh`)
- [ ] Bump `src/version.c`
- [ ] Self-host bootstrap (`./build.sh --mode=self-host`)
- [ ] Update `docs/self-compilation-report.md`
- [ ] Commit with co-author trailer
