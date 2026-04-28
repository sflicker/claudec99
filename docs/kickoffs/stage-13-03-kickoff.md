# Stage-13-03 Kickoff: Array-to-Pointer Decay

## Spec Summary

Add array-to-pointer decay in expression contexts. When an expression
of array type `T[N]` is used as a value, it decays to `T *` whose
value is the address of the first element. Concretely:

- `int *p = a;` where `int a[N];` should behave like
  `int *p = &a[0];` — `p` and `a` refer to the same first element.
- A local array passed as an argument to a `T *` parameter passes
  the array's base address.
- Array names remain non-assignable: `a = b;` and `a = 0;` are
  rejected.

The codegen rule is to emit the array base address (via `lea`)
instead of loading from the stack slot.

## Out of scope (kept as in 13-02)

Pointer arithmetic, pointer difference, multi-dimensional arrays,
`int a[]`-style parameters, `sizeof`, string literals, array
initializers.

## Spec issues flagged

- Title says "Stage 13.4" but filename and content correspond to
  stage 13-03 (the prior milestone was 13-02). The skill rule takes
  the label from the filename → `stage-13-03`.
- Typos: "generatec" → "generated"; "in these context" → "in these
  contexts"; one expected-value comment is missing `//`.

None of these issues block implementation.

## What changes from Stage 13-02

In 13-01 an `AST_VAR_REF` of a local array was hard-rejected as a
value ("cannot use array variable as a value"). In 13-02 array
indexing (`a[i]`) was added but the bare-`a` rejection was kept.
This stage replaces the rejection with decay codegen: an array
`AST_VAR_REF` in a value context evaluates to the array's base
address (via `lea`), with result type `pointer-to-element`.

The whole-array assignment ban stays. The LHS-array rejection in
`AST_ASSIGNMENT` already covers `a = b;` and `a = 0;` because it
inspects the LHS by name *before* evaluating the RHS.

## Planned Changes

| Component | File | Change |
|---|---|---|
| Tokenizer | — | None |
| Parser | — | None |
| AST | — | None |
| Codegen | `src/codegen.c` | `codegen_expression(AST_VAR_REF)`: when `lv->kind == TYPE_ARRAY`, emit `lea rax, [rbp - offset]` and set `result_type = TYPE_POINTER`, `full_type = type_pointer(element)` instead of erroring. `expr_result_type(AST_VAR_REF)`: return `TYPE_POINTER` for arrays. Leave the array-LHS assignment rejection alone. |
| Tests | `test/valid/`, `test/invalid/` | 4 valid (per spec): pointer init from array, pass int array to int* param, char array decay, long array decay. 1 new invalid: `a = 0;` (whole-array assignment to scalar). The `a = b;` invalid case is already covered by `test_invalid_35`. |
| Grammar doc | `docs/grammar.md` | No change — grammar untouched. |
| Milestone / transcript | `docs/milestones/`, `docs/sessions/` | Stage milestone summary and transcript. |

## Implementation Order

1. Codegen: array decay in `AST_VAR_REF` value path; `expr_result_type` array case.
2. Tests: 4 valid, 1 invalid (per spec).
3. Build + run all suites; verify no regressions.
4. Milestone + transcript.
5. Commit.
