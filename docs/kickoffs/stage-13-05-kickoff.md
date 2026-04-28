# Stage-13-05 Kickoff: Pointer Subscript Expressions

## Spec Summary
Add pointer subscript `p[i]` with semantics `*(p + i)`:
  - subscript base may be array OR pointer (preserve array case)
  - index must be integer
  - result is an lvalue of the pointed-to type
  - read and write through pointer indexing both supported

## What Must Change vs. Stage 13-04

The codegen path already handles pointer-base subscripts: `emit_array_index_addr`
was extended in Stage 13-03 to accept `lv->kind == TYPE_POINTER` so that a
pointer parameter receiving a decayed array could be indexed with `p[i]`. That
same path also covers a pointer local initialized from an array (`int *p = a;`)
or from pointer arithmetic (`int *p = a + 1;`).

So Stage 13-05 is mostly formalization:
  - `parse_postfix` already accepts an `AST_VAR_REF` base, regardless of
    whether the variable is an array or a pointer — the comment claiming
    "pointer indexing is out of scope" is now stale and should be updated.
  - All required runtime semantics (`*(p + i)`), error messages, and
    integer-index enforcement are already in place.
  - The remaining work is tests and documentation.

I compiled the four valid and two invalid spec snippets against the current
build; they produce exit codes 15 / 30 / 15 and the expected rejection
messages, with no source changes.

## Spec Issues Flagged

1. **Typo in third valid test.** The body has
   `return a[1] +ap[2];` which is not valid C. Given
   `p = a + 1; p[0] = 7; p[1] = 8;` and an expected exit code of `15`, the
   intended expression is `return a[1] + a[2];` (or equivalently
   `return p[0] + p[1];`). Implementing as `return a[1] + a[2];`.
2. **"Index Expression must be integer type."** `char` / `short` indices
   already promote to `int` in `expr_result_type`, so the existing
   `TYPE_INT || TYPE_LONG` check satisfies the spec. No new validation
   needed.
3. **Error message substrings.** Spec invalid tests expect "subscript
   base is not array or pointer" and "subscript index must be an
   integer." The current codegen emits "subscript base 'X' is not an
   array or pointer" and "array subscript index must be an integer" —
   both contain the spec fragments, so the existing wording is kept.

## Planned Changes

1. **Tokenizer** — none.
2. **Parser** — refresh the `parse_postfix` comment to remove the stale
   "pointer indexing is out of scope" claim. No behavior change.
3. **AST** — none. `AST_ARRAY_INDEX` is reused per the spec's
   implementation note.
4. **Code generator** — none.
5. **Tests (valid)** — three new tests:
   - `test_ptr_subscript_int_3__15.c`
   - `test_ptr_subscript_char_2__30.c`
   - `test_ptr_subscript_offset__15.c` (typo-fixed)
6. **Tests (invalid)** — two new tests:
   - `test_invalid_40__subscript_base_not_array_or_pointer.c`
   - `test_invalid_41__subscript_index_must_be_an_integer.c`
7. **Grammar (`docs/grammar.md`)** — no grammar change required.
8. **Docs** — kickoff (this file), milestone, session transcript.
9. **Commit** — single commit.
