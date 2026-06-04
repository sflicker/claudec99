# Milestone Summary

## Stage 91: Address-of Member Lvalues - Complete

stage-91 extends the unary `&` (address-of) operator to accept any addressable lvalue, enabling code like `&s.member`, `&p->member`, and `&arr[i].member`.

- Tokenizer: No changes.
- Grammar/Parser: No grammar changes; `&` already applies to unary expressions. Extended lvalue check in `parse_unary` to accept `AST_MEMBER_ACCESS` and `AST_ARROW_ACCESS` in addition to existing `AST_VAR_REF` and `AST_ARRAY_INDEX`.
- AST/Semantics: No AST changes; existing member-access node types reused.
- Codegen: Added `struct_field_type()` helper to convert `StructField` descriptor to `Type*`. Extended `AST_ADDR_OF` emit block to dispatch on operand type: `AST_MEMBER_ACCESS` calls `emit_member_addr()`; `AST_ARROW_ACCESS` calls `emit_arrow_addr()`. Both set result type to pointer-to-field. The existing helpers leave the field address in rax.
- Tests: Two new integration-style tests added—dot form (`&s.x`) and arrow form (`&sp->x`)—both exit 42. Full suite: 1302 passed (819 valid, 237 invalid, 82 integration, 43 print_ast, 100 print_tokens, 21 print_asm).
- Docs: Grammar and README updated to reflect the new capability.
- Notes: The lvalue check is semantic, not syntactic, so no grammar update was needed. The existing member-access emission already computes field addresses correctly.
