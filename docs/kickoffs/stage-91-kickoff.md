# Stage 91 Kickoff: Address-of Member Lvalues

## Summary

Extend the unary `&` operator to accept any addressable lvalue, not just identifiers or array subscripts. This allows expressions like `&s.member`, `&p->member`, and `&arr[i].member` to compile correctly. The operator must verify the operand is an lvalue and emit the address of that expression.

## Required Tokenizer Changes

None. The lexer already recognizes the `&` operator.

## Required Parser Changes

Extend the address-of lvalue check in `src/parser.c` to accept:
- `AST_MEMBER_ACCESS` (dot notation: `&s.member`)
- `AST_ARROW_ACCESS` (arrow notation: `&p->member`)

In addition to the currently allowed:
- `AST_VAR_REF` (identifiers)
- `AST_ARRAY_INDEX` (array subscripts)

## Required AST Changes

None. The existing AST node types for member access (`AST_MEMBER_ACCESS`, `AST_ARROW_ACCESS`) are sufficient.

## Required Code Generation Changes

In `src/codegen.c`, extend the `AST_ADDR_OF` emit block to handle:
- `AST_MEMBER_ACCESS` — delegate to existing `emit_member_addr` helper
- `AST_ARROW_ACCESS` — delegate to existing `emit_arrow_addr` helper

Both helpers leave the field address in `rax`, which is the expected result for address-of expressions.

## Test Plan

1. **test/valid/test_addr_of_member_dot__42.c** — Verify `&s.x` works for struct member access
2. **test/valid/test_addr_of_member_arrow__42.c** — Verify `&sp->x` works for pointer-to-struct member access

Both tests assign the member address to a pointer and dereference it to verify correctness.

## Implementation Order

1. Parser changes (lvalue check extension)
2. Code generation changes (emit cases for member access)
3. Version update (`VERSION_STAGE` in `src/version.c`)
4. Add test cases
