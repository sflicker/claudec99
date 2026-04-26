# Stage-12-06 Kickoff: Null Pointer Constant and Pointer Equality

## Spec Summary
Stage 12-06 introduces the null pointer constant: the integer literal
`0` can be used wherever a pointer of any type is expected — in
declaration initializers, return-from-pointer-function expressions,
and equality comparisons against pointers. It also introduces pointer
equality (`==`, `!=`) with strict type rules: pointer-vs-pointer
comparisons require equal type chains; pointer-vs-`0` is always
allowed; pointer vs any non-zero integer is rejected; relational
pointer comparisons remain out of scope. All comparisons keep
returning `int`.

## Spec issues / clarifications
1. Typo: the supported operations list under "Pointer Equality" reads
   `==`, `!-` — should be `==`, `!=`.
2. Typo: "Pointer T vs non-zero **interger**" → "integer".
3. Missing close paren in "All comparisons return `int` (0 or 1,
   consistent with existing behavior".
4. Existing test conflict: `test/invalid/test_invalid_27__returning_non_pointer.c`
   currently expects `int *f() { return 0; }` to be rejected. Stage
   12-06 makes that case valid; the test must be removed. Its purpose
   is already covered by `test_invalid_24` which uses `return 1;`.
5. Out-of-scope edges: the spec names three contexts where `0` may be
   used as a null pointer constant: declaration init, pointer-function
   return, comparison. It is silent about `p = 0` (plain reassignment)
   and `f(0)` for a pointer parameter. Implementation will handle only
   the three contexts spec lists. Plain assignment already accepts an
   integer RHS for pointer locals (no type check today), so `p = 0;`
   continues to compile by accident and is not strengthened here.
6. The lexer stores integer literal values without the `L`/`l` suffix,
   so a check on `AST_INT_LITERAL.value == "0"` accepts both `0` and
   `0L`. C99 treats both as null pointer constants; harmless to allow.

## What changes from Stage 12-05
- Codegen only:
  - Add `is_null_pointer_constant(ASTNode*)` helper.
  - `AST_DECLARATION`: relax pointer-LHS / non-pointer-RHS rejection
    when RHS is the null pointer constant.
  - `AST_RETURN_STATEMENT`: relax pointer-return / non-pointer-expr
    rejection when expression is the null pointer constant.
  - `AST_BINARY_OP` `==`/`!=`: detect pointer operands; require
    chain-equal pointers, allow null pointer constant on either side,
    reject pointer vs non-zero integer.
  - `expr_result_type`: report `TYPE_POINTER` for pointer var-refs and
    `AST_ADDR_OF` so the binary-op pre-walk selects the 64-bit `cmp`.
- No tokenizer / parser / AST / pretty-printer changes.
- Grammar unchanged (spec says "**NONE**").

## Planned Changes
1. **Tokenizer** — none.
2. **Parser** — none.
3. **AST** — none.
4. **Code generator** (`src/codegen.c`)
   - Add `is_null_pointer_constant(ASTNode*)`.
   - Update `expr_result_type`:
     - `AST_VAR_REF` for a pointer local → `TYPE_POINTER`.
     - `AST_ADDR_OF` → `TYPE_POINTER`.
   - `AST_DECLARATION`: when LHS is pointer and RHS is the null pointer
     constant, accept the init (use the 8-byte store path).
   - `AST_RETURN_STATEMENT`: when return type is pointer and the
     expression is the null pointer constant, accept the return (no
     conversion emitted; `mov eax, 0` already zero-extends to `rax`).
   - `AST_BINARY_OP` for `==`/`!=`:
     - If either side's pre-walked type is `TYPE_POINTER`, force common
       width to 64-bit and skip integer `movsxd` widening on pointer
       operands.
     - After both sides are evaluated, validate:
       - both pointers → `pointer_types_equal` required.
       - pointer + integer → integer side must be null pointer constant.
       - else error.
     - Result type stays `TYPE_INT`.
5. **Pretty-printer** — none.
6. **Tests**
   - Valid (`test/valid/`):
     - `test_null_pointer_init__0.c` — `int *p = 0; return 0;`
     - `test_null_pointer_return__1.c` — function returns 0 as `int *`,
       compared back to 0 in main.
     - `test_pointer_eq_same__1.c` — same address, `p == q` is 1.
     - `test_pointer_neq_diff__1.c` — different addresses, `p != q` is 1.
     - `test_pointer_eq_null__1.c` — `int *p = 0; p == 0`.
     - `test_pointer_neq_null__1.c` — non-null pointer `p != 0`.
     - `test_null_eq_pointer__1.c` — `0 == p` (commuted).
   - Invalid (`test/invalid/`):
     - `test_invalid_29__assigning_non_pointer_to_pointer.c` —
       `int *p = 5;`
     - `test_invalid_30__incompatible_pointer.c` — `int *` compared
       with `char *`.
     - `test_invalid_31__assigning_non_pointer_to_pointer.c` — pointer
       compared with non-zero integer (reuses the existing
       `assigning_non_pointer_to_pointer` error fragment).
   - Delete `test/invalid/test_invalid_27__returning_non_pointer.c`
     (now contradicts the spec).
7. **Documentation** — `docs/grammar.md` unchanged (no grammar diff).
8. **Commit** at the end with a concise message.
