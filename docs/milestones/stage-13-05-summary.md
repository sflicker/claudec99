# Stage-13-05 Milestone: Pointer Subscript Expressions

## Completed
- Pointer subscript `p[i]` is now formally part of the language. The
  same `AST_ARRAY_INDEX` node and same codegen path service both array
  and pointer bases:
  - array base: address is `lea` of the array's stack slot.
  - pointer base: address is the value loaded from the pointer local.
  - Both paths add `index * sizeof(element)` after sign-extending the
    index to 64 bits.
- Read and write through pointer indexing both work, including for a
  pointer initialized from an offset into an array (`int *p = a + 1;
  p[0] = 7;`), confirming integration with stage 13-04 pointer
  arithmetic.
- Element types `char`, `short`, `int`, `long`, and pointer-to-T are
  all supported by the existing element-width load/store switch.
- Invalid cases continue to be rejected with their existing messages:
  - subscript on a non-array, non-pointer local → "subscript base 'X'
    is not an array or pointer".
  - non-integer subscript index (e.g. a pointer) → "array subscript
    index must be an integer".
- Stage 13-05 required no tokenizer, AST, or codegen changes; the
  pointer-base path was already present (added in 13-03 to support
  pointer parameters receiving a decayed array). The only source
  change is a parser comment refresh in `parse_postfix` to drop the
  stale "pointer indexing is out of scope" claim.

## Files Changed
- `src/parser.c` — refreshed `parse_postfix` comment.
- `test/valid/` — 3 new tests:
  - `test_ptr_subscript_int_3__15.c`
  - `test_ptr_subscript_char_2__30.c`
  - `test_ptr_subscript_offset__15.c`
- `test/invalid/` — 2 new tests:
  - `test_invalid_40__is_not_an_array_or_pointer.c`
  - `test_invalid_41__subscript_index_must_be_an_integer.c`

## Test Results
- Valid: 228 / 228 pass.
- Invalid: 40 / 40 pass.
- Print-AST: 19 / 19 pass.
- No regressions.
