# Stage-13-04 Kickoff: Pointer Arithmetic

## Spec Summary

Add pointer arithmetic for the additive operators:

- `T * + integer → T *`
- `integer + T * → T *`
- `T * - integer → T *`

The integer side is scaled by `sizeof(*p)` (the pointed-to type
size) before being added to / subtracted from the pointer.

These combinations are explicitly rejected:
- `T * + T *`
- `integer - T *`

Out of scope: pointer − pointer, pointer comparisons (other than
the existing `==` / `!=`), `void *` arithmetic, bounds checking.

## Spec issues flagged

- Heading still reads "Stage 13.4". Filename label is `stage-13-04`.

Otherwise the spec is unambiguous.

## What changes from Stage 13-03

- `AST_ADDR_OF` currently restricts its operand to `AST_VAR_REF` in
  both the parser and codegen. The fourth spec test uses
  `long *p = &a[2];`, so the parser and codegen must additionally
  accept `AST_ARRAY_INDEX` as the operand of `&`.
- `AST_BINARY_OP` codegen currently rejects every pointer operand
  except in `==` / `!=`. Extend `+` and `-` to accept the valid
  pointer-arithmetic combinations and reject the invalid ones.
- `expr_result_type(AST_BINARY_OP)` for `+` / `-` must report
  `TYPE_POINTER` when one operand is a pointer, so dereference and
  assignment paths that consume `p + n` see the correct chain.
- The deref-LHS path in `AST_ASSIGNMENT` and the rvalue `AST_DEREF`
  path already evaluate their operand expression and use its
  `full_type`, so they need no changes once `p + 1` reports the
  right type.

## Planned Changes

| Component | File | Change |
|---|---|---|
| Tokenizer | — | None |
| Parser | `src/parser.c` | Allow `&` operand to be `AST_ARRAY_INDEX` (in addition to `AST_VAR_REF`). |
| AST | — | None |
| Codegen | `src/codegen.c` | `expr_result_type` for `+` / `-`: return `TYPE_POINTER` when one operand is pointer. `AST_BINARY_OP` codegen: classify pointer arithmetic (`ptr ± int`, `int + ptr`); reject `ptr + ptr` and `int - ptr`; scale the integer side by `sizeof(*p)` and emit a 64-bit `add` / `sub`. `AST_ADDR_OF` codegen: when operand is `AST_ARRAY_INDEX`, use `emit_array_index_addr` and tag pointer-to-element. |
| Tests | `test/valid/`, `test/invalid/` | 4 valid (`int*+1`, `long*+1`, `1+char*`, `long*-1`) + 2 invalid (`p+q`, `1-p`). |
| Grammar | — | No change. |
| Docs | `docs/milestones/`, `docs/sessions/` | Stage milestone summary and transcript. |

## Implementation Order

1. Parser: extend `&` to accept `AST_ARRAY_INDEX`.
2. Codegen: addr-of of array-index.
3. Codegen: pointer arithmetic in `+` / `-` and `expr_result_type`.
4. Tests (4 valid + 2 invalid).
5. Build, run all suites.
6. Milestone + transcript.
7. Commit.
