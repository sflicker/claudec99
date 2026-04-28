# Stage-13-03 Milestone: Array-to-Pointer Decay

## Completed
- An array name in a value context decays to `T *` (pointer to the
  element type). Codegen emits `lea rax, [rbp - offset]` instead of
  loading from the slot, and tags the result `TYPE_POINTER` with a
  freshly-built `pointer-to-element` Type chain.
- `expr_result_type(AST_VAR_REF)` reports `TYPE_POINTER` for an array
  local so binary operators against an array name pick up the
  pointer typing path.
- Whole-array assignment is still rejected: `a = b;` and `a = 0;`
  both fail at the existing array-LHS check in `AST_ASSIGNMENT`.
- Array decay flows through the existing pointer-init and pointer-
  argument paths unchanged: `int *p = a;` writes the array's base
  address into `p`'s slot, and `first(a)` to `int first(int *p)`
  passes the base address as the argument.
- Subscript codegen extended to accept a pointer-local base. This
  was needed once a pointer parameter could receive a decayed array
  and be indexed with `p[0]` in spec test cases. Element-type lookup
  is identical between array and pointer locals
  (`lv->full_type->base`); only the base-address materialisation
  differs (`lea` vs. `mov`).

## Files Changed
- `src/codegen.c`
  - `expr_result_type` AST_VAR_REF — array case returns
    `TYPE_POINTER`.
  - `expr_result_type` AST_ARRAY_INDEX — accept pointer base.
  - `codegen_expression` AST_VAR_REF — array case emits `lea` and
    sets pointer result type / full Type chain.
  - `emit_array_index_addr` — accept pointer base; emit `mov` to
    load the pointer value as the base address.
- `test/valid/` — 4 new tests:
  - `test_array_decay_pointer_init__37.c`
  - `test_array_decay_int_param__42.c`
  - `test_array_decay_char_param__12.c`
  - `test_array_decay_long_param__42.c`
- `test/invalid/` — 1 new test:
  - `test_invalid_37__arrays_are_not_assignable.c` (`a = 0;`)

## Spec Note
- The spec heading reads "Stage 13.4" but the filename and content
  correspond to stage 13-03; this is recorded as a doc-only typo.
  The skill rule derives the label from the filename.
- The 13-03 spec's tests index a pointer parameter (`p[0]`).
  Stage 13-02's milestone listed pointer indexing as out of scope,
  so this stage extends subscript to accept pointer locals — this
  is a small, implicit scope expansion required to compile the
  spec's own tests.

## Test Results
- Valid: 221 / 221 pass.
- Invalid: 36 / 36 pass.
- Print-AST: 19 / 19 pass.
- No regressions.
