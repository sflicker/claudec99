# stage-80 Transcript: Prefix/Postfix ++/-- on General Lvalues

## Summary

Stage 80 extends prefix and postfix increment/decrement operators (++/--) to work on any modifiable lvalue expression, not just bare identifiers. This unblocks two critical self-compile idioms: `parent->children[parent->child_count++] = child` (from ast.c) and `g->data[g->len++] = c` (from preprocessor.c). The operand must be a modifiable lvalue (bare identifier, struct/union member via . or ->, array subscript, pointer dereference, or chains thereof) or a pointer type; const-qualified and non-lvalue operands are rejected. Postfix yields the old value; prefix yields the new value. Pointer ++/-- continues to scale by pointee size.

## Changes Made

### Step 1: Parser (src/parser.c)

- Removed the AST_VAR_REF-only check in `parse_postfix` that previously rejected postfix ++/-- on non-identifier expressions.
- Removed the restriction in `parse_unary` (for prefix ++/--) so the operator can attach to any already-parsed unary/postfix expression, not just bare identifiers.
- Lvalue validity is now deferred to code generation, where the semantic check `"operand of ++/-- must be a modifiable lvalue"` is enforced per spec.

### Step 2: Codegen (src/codegen.c)

- Added `codegen_inc_dec_general()` helper to handle operands of kind AST_ARRAY_INDEX, AST_MEMBER_ACCESS, AST_ARROW_ACCESS, and AST_DEREF.
- The helper computes the element address once via existing stage-79 address-emission helpers (`emit_array_index_addr`, `emit_member_addr`, `emit_arrow_addr`, and the deref-pointer path), which preserve the base on the stack across index/sub-expression evaluation so nested forms (e.g., `g->data[g->len++]`) are handled correctly.
- Loads the value with the appropriate element width (sign-extending integers, matching the existing rvalue load paths).
- Adjusts the value: pointers increment/decrement by pointee size; integers by 1.
- Stores the result back through the original address.
- Returns the old value (postfix) or new value (prefix) as required.
- Updated the prefix and postfix branches in `codegen_expression` to dispatch to `codegen_inc_dec_general()` when the operand is not AST_VAR_REF.
- Updated the identifier fast-path to also reject const-qualified variables (mirroring the assignment rejection path).
- Non-lvalue operands (e.g., integer literals, function calls) now raise diagnostic error `"operand of ++/-- must be a modifiable lvalue"`.

### Step 3: Version

- Bumped VERSION_STAGE from 00790000 to 00800000 in src/version.c.

### Step 4: Tests

#### Valid tests (test/valid/)

- `test_inc_dec_arrow_subscript_index__43.c`: `parent->children[parent->child_count++] = child` pattern.
- `test_inc_dec_dot_subscript_index__42.c`: `g.data[g.len++] = c` pattern with dot member access.
- `test_postfix_inc_arrow_member__42.c`: `p->cap++` pattern (corrected typo from spec test).
- `test_prefix_dec_arrow_member__42.c`: `--p->n` pattern.
- `test_postfix_inc_array_elem__42.c`: `xs[1]++` pattern.
- `test_deref_postfix_inc__42.c`: `(*p)++` pattern.
- `test_postfix_inc_old_value__43.c`: `y = x++` returns old value.
- `test_prefix_inc_new_value__42.c`: `y = ++x` returns new value.

#### Invalid tests (test/invalid/)

- `test_postfix_inc_int_literal__not_an_lvalue.c`: `42++` is rejected.
- `test_postfix_inc_paren_sum__not_an_lvalue.c`: `(a+b)++` is rejected (non-lvalue).
- `test_postfix_inc_const__assignment_to_const.c`: `const int x; x++` is rejected (const lvalue).

## Final Results

All 1244 tests pass (776 valid, 234 invalid, 71 integration, 43 print-AST, 99 print-tokens, 21 print-asm). No regressions.

## Session Flow

1. Read stage spec and supporting reference documentation.
2. Reviewed relevant parser and codegen code (++/-- handling, lvalue semantics, address-emission helpers).
3. Identified the parser restriction blocking general lvalues and the code generation strategy.
4. Implemented parser changes to remove AST_VAR_REF-only restriction.
5. Implemented `codegen_inc_dec_general()` and dispatch logic in `codegen_expression`.
6. Updated version constant.
7. Added 8 valid tests covering the new lvalue forms and value-semantics requirements.
8. Added 3 invalid tests covering rejection of non-lvalues and const-qualified operands.
9. Built compiler and ran full test suite; all 1244 tests pass.
10. Verified stage-79 self-compile idioms now compile and run correctly.

## Notes

- The well-tested identifier codegen path (which handles static variables) was kept unchanged except for the const-rejection addition, minimizing regression risk.
- Address-emission helpers from stage 79 were reused rather than introducing a new unified lvalue-address function; this reduced complexity and leveraged existing, battle-tested code.
- The spec's "Member postfix increment" test example contained a typo (`p->cat++` instead of `p->cap++`) and a missing closing brace. These were corrected in the added test `test_postfix_inc_arrow_member__42.c`.
- The spec's invalid test used `const x = 1` (implicit int, not supported by this compiler). The added invalid test uses `const int x = 1; x++;` to genuinely exercise const-lvalue rejection.
