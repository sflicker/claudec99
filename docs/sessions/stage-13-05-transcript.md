# stage-13-05 Transcript: Pointer Subscript Expressions

## Summary

This stage formalizes pointer subscript expressions `p[i]` with the
semantics `*(p + i)`. The same `AST_ARRAY_INDEX` node and codegen
path service both array and pointer bases: the array case takes the
slot's address with `lea`, the pointer case loads the slot's value;
both then add `index * sizeof(element)` after sign-extending the
index. The element-width load/store switch already covers `char`,
`short`, `int`, `long`, and pointer-to-T elements.

The pointer-base path was added back in stage 13-03 to support
pointer parameters receiving a decayed array; stage 13-05 makes that
behaviour part of the language for any pointer expression that
parses as an identifier base. No tokenizer, AST, or codegen change
was required — only a parser comment refresh and tests.

## Changes Made

### Step 1: Parser

- `parse_postfix` doc-comment refreshed to drop the stale
  "pointer indexing is out of scope" note. No code change.

### Step 2: Tests

- `test/valid/test_ptr_subscript_int_3__15.c` — `int *p = a; p[i]`
  read/write.
- `test/valid/test_ptr_subscript_char_2__30.c` — char element type.
- `test/valid/test_ptr_subscript_offset__15.c` — `int *p = a + 1;`
  exercises pointer subscript on top of pointer arithmetic.
- `test/invalid/test_invalid_40__is_not_an_array_or_pointer.c` —
  `return x[0];` where `x` is `int`.
- `test/invalid/test_invalid_41__subscript_index_must_be_an_integer.c`
  — `return p[p];` rejected for non-integer index.

## Final Results

All suites pass: 228 valid (+3), 40 invalid (+2), 19 print-AST. No
regressions.

## Session Flow

1. Read the stage spec and supporting files.
2. Surveyed the existing array-index path; confirmed `emit_array_index_addr`
   already handles `TYPE_POINTER` bases (added in 13-03).
3. Compiled the spec snippets manually against the unchanged build to
   verify the implementation already covered all required semantics.
4. Wrote and saved the kickoff document; flagged a typo in the spec's
   third valid test (`+ap[2]`) and confirmed intent with the user.
5. Refreshed the `parse_postfix` comment.
6. Added 3 valid and 2 invalid tests; renamed the first invalid test
   so the filename fragment matches the codegen's actual error string.
7. Built and ran all suites — all green.
8. Wrote milestone and transcript.

## Notes

- Spec test 3 had a typo (`return a[1] +ap[2];`); implemented as
  `return a[1] + a[2];` per user confirmation.
- No grammar change required — `<postfix_expression>` already permits
  the subscript suffix regardless of base type.
- Error-message fragments from the codegen are preserved; the invalid
  tests are named to match those substrings.
