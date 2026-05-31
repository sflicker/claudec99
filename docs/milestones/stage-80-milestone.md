# Milestone Summary

## stage-80: Prefix/Postfix ++/-- on General Lvalues - Complete

stage-80 extends prefix/postfix increment and decrement operators to work on any modifiable lvalue (struct/union member access, array subscripts, pointer dereferences, and chains thereof), unblocking self-compile idioms like `parent->children[parent->child_count++] = child` and `g->data[g->len++] = c`.

- Tokenizer: No changes (++ and -- already tokenized).
- Grammar/Parser: Removed AST_VAR_REF-only restriction in `parse_postfix` and `parse_unary`; lvalue validity now enforced during code generation per spec semantics.
- Codegen: Added `codegen_inc_dec_general()` to handle operands of kind AST_ARRAY_INDEX, AST_MEMBER_ACCESS, AST_ARROW_ACCESS, and AST_DEREF; reused stage-79 address helpers to compute element addresses; loads value with element width, adjusts (pointers scale by pointee size), stores back, yields old value (postfix) or new value (prefix); non-lvalue and const-qualified operands raise diagnostic errors.
- Tests: 8 valid cases (arrow/dot subscript chains, member access, deref, old/new value semantics); 3 invalid cases (literal, non-lvalue expression, const-qualified lvalue). Full suite 1244/1244 pass.
- Docs: README.md updated (stage reference, ++/-- semantics on general lvalues).
- Notes: Kept identifier fast-path unchanged to minimize regression risk; reused existing per-kind address helpers rather than a new unified lvalue-address function. Spec test "Member postfix increment" had a typo (p->cat++ for p->cap++) and missing closing brace — corrected in added test.
