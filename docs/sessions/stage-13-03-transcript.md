# stage-13-03 Transcript: Array-to-Pointer Decay

## Summary

This stage adds array-to-pointer decay in expression contexts. An
expression of array type `T[N]` used as a value now decays to `T *`
whose value is the address of the first element. Codegen emits `lea
rax, [rbp - offset]` instead of a slot load, and the result is
typed `pointer-to-element` so it flows through the existing
pointer-init and pointer-argument paths unchanged.

Whole-array assignment is still rejected: the LHS-array check in
`AST_ASSIGNMENT` runs before the RHS is evaluated and covers both
`a = b;` and `a = 0;`.

To make the spec's own tests compile, subscript codegen was
extended to accept a pointer-local base — `p[0]` where `p` is a
pointer parameter computes `*(p + 0 * sizeof(*p))` using the
pointer's stored value as the base address. Stage 13-02 had listed
pointer indexing as out of scope; this stage adds the minimum
support required by the spec's tests.

## Changes Made

### Step 1: Code Generator

- `expr_result_type` AST_VAR_REF: return `TYPE_POINTER` for a local
  whose kind is `TYPE_ARRAY`, so binary ops against an array name
  pick up the pointer-typing path.
- `expr_result_type` AST_ARRAY_INDEX: accept pointer-local bases by
  reading the element type from `lv->full_type->base` for both
  array and pointer kinds.
- `codegen_expression` AST_VAR_REF: when the local is an array,
  emit `lea rax, [rbp - offset]`, set `result_type = TYPE_POINTER`
  and `full_type = type_pointer(lv->full_type->base)` (a fresh
  pointer Type wrapping the element type). The earlier
  hard-rejection of array values is removed.
- `emit_array_index_addr`: accept a pointer-local base. For an
  array the slot's address is the base (`lea`); for a pointer the
  slot's value is the base (`mov rax, [rbp - offset]`). Element
  type and the rest of the address computation are identical.

### Step 2: Tests

- `test/valid/test_array_decay_pointer_init__37.c` — pointer init
  from array, write through `a[0]`, read through `*p`.
- `test/valid/test_array_decay_int_param__42.c` — pass `int a[2]`
  to `int first(int *p)` and index with `p[0]`.
- `test/valid/test_array_decay_char_param__12.c` — same shape with
  `char` element type.
- `test/valid/test_array_decay_long_param__42.c` — same shape with
  `long` element type.
- `test/invalid/test_invalid_37__arrays_are_not_assignable.c` —
  `a = 0;` rejected. The `a = b;` case is already covered by
  `test_invalid_35`.

## Final Results

All suites pass: 221 valid (+4), 36 invalid (+1), 19 print-AST.
No regressions.

## Session Flow

1. Read the stage spec and noted the "Stage 13.4" heading typo.
2. Surveyed type/codegen/test layout to scope the change.
3. Wrote and saved the kickoff document.
4. Implemented the AST_VAR_REF decay path and the
   `expr_result_type` array case in codegen.
5. Built the compiler and ran all suites; 3 valid tests failed
   (pointer-parameter indexing not yet supported).
6. Extended `emit_array_index_addr` and `expr_result_type` to
   accept pointer-local bases.
7. Re-ran all suites — 221 / 36 / 19 pass, no regressions.
8. Wrote milestone and transcript.

## Notes

The spec heading reads "Stage 13.4" but the filename is
`ClaudeC99-spec-stage-13-03-array-to-pointer-decay.md`. The skill
rules derive `STAGE_LABEL` from the filename, so all artifacts use
`stage-13-03`.

The spec lists "Out of scope" items but does not list pointer
indexing. Its test cases (`p[0]` inside `first(int *p)`) require
pointer indexing to compile, so this stage adds the minimum
support: a pointer-local base is treated like an array base except
the address is loaded from the slot rather than computed via `lea`.
