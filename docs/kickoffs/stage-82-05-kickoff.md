# Stage 82-05 Kickoff

## Summary of the Spec

Stage 82-05 addresses interaction with pointer compatibility diagnostics when const comes from member types. The goal is to ensure existing const pointer warning/error behavior continues to work correctly in complex scenarios involving:

- Pointer-to-const members (e.g., `const char *name`)
- Const scalar members (e.g., `const int code`)
- Volatile members
- sizeof expressions with const type names
- Const-discard warnings from member access
- Assignment to members of const objects (new scenario)

Key principle: Member types must store qualifiers exactly as parsed (pointer-to-const, not const-pointer). Member access expressions inherit the member's type including qualifiers. When the base object is const-qualified, all member access is non-modifiable regardless of the member type itself.

## Required Tokenizer Changes

None. All required tokens (const, volatile, struct, member operators) are already recognized.

## Required Parser Changes

None. The parser already stores full_type with qualifiers for member declarations.

## Required AST Changes

None. The AST already carries is_const on AST_DECLARATION nodes.

## Required Code Generation Changes

1. Add `member_base_is_const(CodeGen *cg, ASTNode *node)` helper function in `src/codegen.c`:
   - Walks a member-access AST chain (`.member` or `->member`) to find the root struct/union variable
   - Returns 1 if the root variable is const-qualified, 0 otherwise

2. Apply const-base checking in `AST_ASSIGNMENT` + `AST_MEMBER_ACCESS` path:
   - After existing `f->is_const` check, call `member_base_is_const()` on the assignment target
   - Reject assignment if the base object is const

3. Apply const-base checking in `AST_ASSIGNMENT` + `AST_ARROW_ACCESS` path:
   - Check if the pointer's pointee type is const-qualified
   - Reject assignment through const-qualified pointee

4. Update `src/version.c`:
   - Bump VERSION_STAGE from "00820400" to "00820500"

## Test Plan

### Valid Tests
- `test_struct_member_const_discard_warning__0.c`:
  - Struct with `const char *name` member
  - Assign member to `char *p`
  - Expect warning (const-discard) and exit code 0

### Invalid Tests
- `test_struct_const_base_member_assign__assignment_to_const_object.c`:
  - Declare `const struct S s;` with non-const member `x`
  - Attempt `s.x = 1;`
  - Must be rejected (assignment to const object member)

### Integration Tests
- `struct_member_const_discard_werror/`:
  - Const-discard from member with `-Werror`
  - Expect compilation error

Additional coverage from spec examples:
- Const scalar member with aggregate initializer (valid, expect success)
- Volatile member (valid, expect success)
- sizeof(const type-name) (valid, expect success)
- Assignment to const member (invalid, expect rejection)
- Assignment through const char array member (invalid, expect rejection)

## Planned File Changes

- `src/codegen.c`: Add `member_base_is_const()` helper, apply in assignment paths
- `src/version.c`: Version bump
- `test/valid/test_struct_member_const_discard_warning__0.c`: New
- `test/invalid/test_struct_const_base_member_assign__assignment_to_const_object.c`: New
- `test/integration/struct_member_const_discard_werror/`: New integration test with `.cflags` containing `-Werror`

## Spec Issues and Decisions

1. **Typo in "Const char pointer member" test**: Contains stray apostrophe in `s.name = "ClaudeC99"'` — will create corrected version without the trailing apostrophe.

2. **"Const-discard from member" labeled Invalid but produces warning**: The spec shows this in the invalid section but with a comment "INVALID or Warning". This produces a warning diagnostic in stage 82-01/02, so it's treated as a valid test with expected warning exit code 0, plus an integration test with `-Werror` to verify error behavior.

3. **Member type storage guarantee**: Confirmed that member types are stored with qualifiers preserved (e.g., `const char *` not `char *`).

## Implementation Order

1. Add `member_base_is_const()` helper in codegen.c
2. Apply const-base check in AST_MEMBER_ACCESS assignment path
3. Apply const-base check in AST_ARROW_ACCESS assignment path
4. Update version.c
5. Create test files
6. Run test suite and verify diagnostics
