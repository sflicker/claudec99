# Milestone Summary

## Stage 97: Designated Initializers — Complete

Stage 97 ships C99 designated initializers (`.member = value` for struct fields and `[index] = value` for array elements) in brace-enclosed initializer lists, enabling sparse and out-of-order field/element initialization with automatic zero-fill for skipped positions.

- **AST**: Added `AST_DESIGNATED_INIT` node type; discriminated by `value != NULL` (member designator) vs. `value == NULL` (array designator with index in `byte_length`); always has one child (the value initializer).
- **Parser**: New `parse_initializer_element()` helper dispatches on `.IDENT =` and `[const_expr] =` before falling through to `parse_initializer`; detects chained designators (`.a.b` or `[x][y]`) and rejects with a diagnostic; forward-declared `eval_case_const_expr` for use in array index parsing.
- **Codegen — local struct**: Rewrote `emit_local_struct_init` with cursor-pattern field tracking and member-name lookup; struct is unconditionally zero-filled by caller, then only designated/positioned fields are overwritten.
- **Codegen — local array**: Replaced positional loop with two-phase: Phase 1 zero-fills entire array; Phase 2 applies initializers with cursor-based index tracking and designator bounds checking.
- **Codegen — global struct**: Rewrote `emit_global_struct` to build a field-index → element map (`slots[MAX_STRUCT_FIELDS_DESIGNATED]`, fixed-size for self-host) before emission; fields are emitted in declared order with looked-up initializer or zero-fill.
- **Codegen — global array**: Rewrote array emit to build element-index → initializer map (`slots[MAX_ARRAY_ELEMS_DESIGNATED]`, fixed-size) with cursor tracking before emission.
- **AST printer**: Added `case AST_DESIGNATED_INIT:` to output `DESIGNATED_INIT(.name)` or `DESIGNATED_INIT([N])` followed by the child.
- **Tests**: 18 new tests — 10 valid (basic/partial/mixed struct, basic/advance/reorder array, global struct/array, array-of-structs, char-literal index), 5 invalid (unknown member, wrong designator type, bounds), 2 print_ast, 1 integration multi-file.
- **All 1501 tests pass** (843 valid, 242 invalid, 85 integration, 45 print_ast, 100 print_tokens, 21 print_asm). Self-host C0→C1→C2 cycle passes cleanly; no bootstrap bugs found.
- **Version**: Bumped VERSION_STAGE to `"00970000"`.
- **Notes**: Chained designators detected and rejected before `=` is consumed (bug fix: initially placed after `=`, which emitted wrong error message). Two-phase array init avoids data loss if designators reorder or skip. Bootstrap caveats observed: VLAs replaced with fixed-size slots arrays; Vec dereference-cast patterns left unchanged (not present in new code).
