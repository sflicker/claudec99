# Stage 37-02 Kickoff

## Summary

Stage 37-02 adds four tests for struct functionality: three invalid tests that verify error detection, and one valid test demonstrating typedef'd self-referential linked list behavior. No grammar changes, no new AST nodes, no new tokens. Implementation focuses on three focused bug fixes in code generation.

## Tokenizer Changes

None required.

## Parser Changes

None required.

## AST Changes

None required.

## Code Generation Changes

Three focused fixes in `src/codegen.c`:

1. **`emit_arrow_addr` function** — Add handling for `AST_MEMBER_ACCESS` base nodes
   - Currently the arrow operator only handles array subscripts and pointer dereferences
   - Need to support pattern: `n.next->value` where the base is a member access

2. **`AST_SIZEOF_TYPE` case** — Add check to reject sizeof on incomplete struct
   - Incomplete structs have `size == 0`
   - Should emit error when sizeof applied to incomplete type

3. **`AST_DECLARATION` struct path** — Add check to reject variable declaration of incomplete struct type
   - When declaring a variable with struct type, if struct is incomplete (`size == 0`), should emit error

## Test Plan

### Valid Tests
- `test/valid/test_typedef_linked_list_arrow__7.c` — typedef struct with self-referential pointer, demonstrates `n.next->value` pattern

### Invalid Tests
- `test/invalid/test_struct_incomplete_var__has_incomplete_struct_type.c` — declaring variable of incomplete struct type
- `test/invalid/test_struct_sizeof_incomplete__sizeof_applied_to_incomplete.c` — sizeof applied to incomplete struct
- `test/invalid/test_struct_recursive_byval__has_incomplete_struct_type.c` — recursive struct by value (forward-declared in struct body)

Note: Spec contains typos in invalid tests:
- Invalid test 2 has `itn main()` (should be `int`) — will be corrected during implementation
- Invalid test 3 has `Struct Node next` (should be `struct Node next` with lowercase `struct`) — will be corrected during implementation

## Implementation Order

1. Fix `emit_arrow_addr` to handle `AST_MEMBER_ACCESS` base
2. Add sizeof incomplete struct check to `AST_SIZEOF_TYPE` case
3. Add variable declaration incomplete struct check to `AST_DECLARATION` struct path
4. Create all four test files with corrected syntax
5. Run full test suite to verify no regressions

## Ambiguities & Decisions

None identified. The spec is clear on the three code generation fixes needed and the test cases to add.

## Derived Values

- **STAGE_LABEL**: `stage-37-02`
- **STAGE_DISPLAY**: `Stage 37-02`
- **Spec Path**: `docs/stages/ClaudeC99-spec-stage-37-02-additional-struct-tests.md`
