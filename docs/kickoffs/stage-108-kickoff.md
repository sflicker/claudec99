# Stage 108 Kickoff

## Summary of the Spec

Stage 108 adds two new conditional-compilation directives to the preprocessor:

1. **`#elifdef NAME`** — equivalent to `#elif defined(NAME)`. Allows a branch-transition between an active conditional block and `#else`/`#endif`. Standardised in C23 §6.10.1; supported as an extension in GCC 12+ and Clang 13+.

2. **`#elifndef NAME`** — equivalent to `#elif !defined(NAME)`. Identical semantics with inverted condition. Used in system headers and real-world compatibility code.

Both directives are confined to the preprocessor. No lexer, parser, AST, codegen, or header changes are required. Only `src/preprocessor.c` and `src/version.c` are modified. The change is approximately 30 lines of preprocessor logic.

---

## Required Preprocessor Changes

In `src/preprocessor.c`, locate the `preprocess_internal` function. The conditional-directive dispatch currently runs:

```
#ifdef  (line ~1233)
#ifndef (line ~1257)
#if     (line ~1281)
#elif   (line ~1310)
#else   (line ~1347)
#endif  (line ~1370)
```

Insert two new blocks **immediately before the `#elif` block** at line ~1310:

### 1a — `#elifdef NAME`

```c
/* #elifdef NAME  (C23 §6.10.1 / GCC/Clang extension) */
if (strncmp(s + in, "elifdef", 7) == 0 &&
    !isalnum((unsigned char)s[in + 7]) && s[in + 7] != '_') {
    in += 7;
    if (cond_depth == 0) {
        fprintf(stderr, "error: #elifdef without conditional\n");
        free(out.data); free(spliced); exit(1);
    }
    CondFrame *top = &cond_stack[cond_depth - 1];
    if (top->seen_else) {
        fprintf(stderr, "error: #elifdef after #else\n");
        free(out.data); free(spliced); exit(1);
    }
    int cond_val = 0;
    if (top->parent_emitting && !top->branch_taken) {
        while (s[in] == ' ' || s[in] == '\t') in++;
        size_t name_start = in;
        while (s[in] && (isalnum((unsigned char)s[in]) || s[in] == '_'))
            in++;
        size_t name_len = in - name_start;
        cond_val = (macro_find(macros, s + name_start, name_len) != NULL);
    }
    while (s[in] && s[in] != '\n') in++;
    if (top->parent_emitting) {
        if (!top->branch_taken && cond_val) {
            top->emitting = 1;
            top->branch_taken = 1;
        } else {
            top->emitting = 0;
        }
    }
    emitting = top->emitting;
    continue;
}
```

### 1b — `#elifndef NAME`

Insert immediately after the `#elifdef` block (still before `#elif`):

```c
/* #elifndef NAME  (C23 §6.10.1 / GCC/Clang extension) */
if (strncmp(s + in, "elifndef", 8) == 0 &&
    !isalnum((unsigned char)s[in + 8]) && s[in + 8] != '_') {
    in += 8;
    if (cond_depth == 0) {
        fprintf(stderr, "error: #elifndef without conditional\n");
        free(out.data); free(spliced); exit(1);
    }
    CondFrame *top = &cond_stack[cond_depth - 1];
    if (top->seen_else) {
        fprintf(stderr, "error: #elifndef after #else\n");
        free(out.data); free(spliced); exit(1);
    }
    int cond_val = 0;
    if (top->parent_emitting && !top->branch_taken) {
        while (s[in] == ' ' || s[in] == '\t') in++;
        size_t name_start = in;
        while (s[in] && (isalnum((unsigned char)s[in]) || s[in] == '_'))
            in++;
        size_t name_len = in - name_start;
        cond_val = (macro_find(macros, s + name_start, name_len) == NULL);
    }
    while (s[in] && s[in] != '\n') in++;
    if (top->parent_emitting) {
        if (!top->branch_taken && cond_val) {
            top->emitting = 1;
            top->branch_taken = 1;
        } else {
            top->emitting = 0;
        }
    }
    emitting = top->emitting;
    continue;
}
```

---

## Design Notes

**Boundary guard analysis:** The existing `#elif` check uses `strncmp(s + in, "elif", 4)` with a boundary guard `!isalnum(...) && s[in + 4] != '_'`. For `#elifdef`, `s[in + 4]` is `'d'` (alphanumeric), so the condition is false — no collision. Similarly, `#elifndef` has `'n'` at offset 4, also failing the guard. Both new directives are placed before `#elif` for consistency with the `#ifdef`-before-`#if` convention, but placement order is not strictly critical due to the keyword boundaries.

**Inactive-region correctness:** Branch-transition directives are checked before the `if (!emitting)` skip that gates active-only directives. Both new blocks sit in the same pre-emitting-check region, so they correctly update `cond_stack` state even when the surrounding block is inactive — which is required for proper nesting.

**`macro_find` visibility:** The function is already in scope (used by `#ifdef`/`#ifndef` earlier in the same `if` chain). No forward declarations or include changes are needed.

---

## Version Change

In `src/version.c`, change:
```c
#define VERSION_STAGE  "01070000"
```
to:
```c
#define VERSION_STAGE  "01080000"
```

---

## Test Plan

All tests go in `test/valid/` and verify that both directives work correctly in various contexts:

- **`test_elifdef_taken__0.c`** — `#elifdef` branch taken (exit: 0)
- **`test_elifdef_else__0.c`** — `#elifdef` branch not taken, `#else` taken (exit: 0)
- **`test_elifndef_taken__0.c`** — `#elifndef` branch taken (exit: 0)
- **`test_elifndef_false__0.c`** — `#elifndef` branch not taken (exit: 0)
- **`test_elifdef_chain__0.c`** — chained `#elifdef` / `#elifndef` (exit: 0)
- **`test_elifdef_nested__0.c`** — `#elifdef` in nested conditional (exit: 0)

---

## Implementation Order

1. `src/preprocessor.c` — add `#elifdef` block before `#elif`
2. `src/preprocessor.c` — add `#elifndef` block before `#elif`
3. `src/version.c` — bump stage to `"01080000"`
4. Add test files
5. Build, run full test suite, self-host cycle
6. Update documentation (README.md, docs/grammar.md, docs/outlines/checklist.md; run update-supplemental-docs skill)

---

## Ambiguities and Decisions

**Single-file change:** Only `src/preprocessor.c` and `src/version.c` are modified. No lexer, parser, AST, codegen, or header changes are needed.

**Keyword boundary safety:** The `#elifdef` and `#elifndef` keyword boundaries cannot collide with `#elif` due to the isalnum guard at position 4. Placement before `#elif` is a style choice, not a correctness requirement.

**Inactive-region correctness:** The spec explicitly confirms that branch-transition directives update `cond_stack` before the emitting check, ensuring proper nesting even in inactive regions.

---

## Known Issues

None identified. The spec is clear and unambiguous. The change is entirely contained in `src/preprocessor.c` with no grammar or AST effects.
