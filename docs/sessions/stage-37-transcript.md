# stage-37 Transcript: Incomplete Struct Declarations

## Summary

Stage 37 adds support for incomplete struct declarations (forward declarations), enabling two important patterns: self-referential structs via typedef aliases and opaque pointer fields. The implementation modifies `parse_struct_specifier` to create a placeholder type with zero size when a struct tag has no body yet. When the struct body is later defined, the placeholder is updated in-place (mutating size, alignment, and field list) to ensure all typedef entries pointing to it automatically reflect the complete layout. This approach avoids the need for type resolution passes and keeps typedef pointers valid throughout the translation unit.

Non-pointer incomplete struct fields continue to be rejected, maintaining C99 semantics: only pointer-to-incomplete-struct types are permitted as struct members.

## Changes Made

### Step 1: Parser Enhancement

- Modified `parse_struct_specifier` in `src/parser.c`:
  - When encountering a struct tag with no body (forward declaration), create a `type_struct(0, 0)` placeholder instead of erroring.
  - When parsing a struct body definition, check if the tag already exists with `size == 0` (incomplete form).
  - If the placeholder exists, mutate it in-place: update `size`, `alignment`, and `fields` to reflect the complete struct layout.
  - This ensures all typedef entries pointing to the placeholder automatically become valid when the body is defined.
- Added validation in struct field parsing: if a field type is an incomplete struct (non-pointer, with `size == 0`), emit diagnostic error "error: field '%s' has incomplete struct type".

### Step 2: Invalid Test Rename

- Renamed `test/invalid/test_struct_incomplete_member__is_not_defined.c` to `test/invalid/test_struct_incomplete_member__has_incomplete_struct_type.c` to reflect the updated error message.
- Test continues to reject non-pointer incomplete struct fields (e.g., `struct Missing m;`).

### Step 3: Valid Tests Added

- Created `test/valid/test_incomplete_struct_self_ref__7.c`: demonstrates `typedef struct ASTNode ASTNode;` forward declaration followed by body definition with self-referential pointer fields. Struct fields successfully access the typedef'd type. Returns exit code 7.
- Created `test/valid/test_incomplete_struct_forward__5.c`: demonstrates bare `struct Lexer;` forward declaration followed by `struct parser` with opaque pointer field `struct Lexer *lexer;`. Returns exit code 5.

## Final Results

Build: Successful.
Test Results: All 772 tests pass (770 existing + 2 new valid tests).
Regressions: None. Invalid test error message updated; semantics unchanged.

## Session Flow

1. Read stage 37 spec and summary of completed changes.
2. Examined `parse_struct_specifier` implementation in `src/parser.c` to understand placeholder creation and in-place mutation.
3. Reviewed new valid test cases to confirm self-referential and forward-declared struct patterns work.
4. Reviewed invalid test to confirm non-pointer incomplete struct fields are still rejected.
5. Generated milestone summary documenting subsystem changes and test totals.
6. Generated transcript documenting implementation steps and verification.
7. Updated grammar documentation to reflect incomplete struct declaration support.
8. Updated README with new stage reference, updated test totals, and enhanced struct feature description.

## Notes

**In-place Mutation Strategy**: The key design decision is to mutate the placeholder type in-place when the struct body is later defined. This ensures that all typedef entries created during the forward declaration phase automatically point to the complete layout, without requiring type-resolution passes or reference updates. The approach is simple and efficient.

**Field Validation**: Non-pointer incomplete struct fields are explicitly rejected with a diagnostic error. Only pointer-to-incomplete-struct types are allowed, consistent with C99 semantics. This prevents use of incomplete types as direct struct members while permitting opaque pointer fields.
