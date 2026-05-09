# Milestone Summary

## Stage 28-04: Array Typedefs - Complete

stage-28-04 ships support for array typedefs (e.g., `typedef int A[4];`), enabling array types to be aliased and used in variable declarations at both file and block scope.

- **Tokenizer**: No changes.
- **Parser**: Removed blanket rejection of array typedefs in both file-scope and block-scope typedef handlers; added TYPE_ARRAY typedef registration with full array type information. Block-scope variable declarations now recognize typedef'd array types. Multi-declarator lists handle array typedefs correctly. Relaxed array subscript base check to allow AST_DEREF for patterns like `(*p)[0]`.
- **AST/Semantics**: Typedef entries now support array types with stored element type and length.
- **Codegen**: Added AST_DEREF base handling in `emit_array_index_addr` to support pointer-to-array subscripting.
- **Tests**: 5 new tests added covering basic typedef arrays, multi-declarators, sizeof, and pointer-to-typedef-array patterns. Full suite 728/728 pass.
- **Docs**: Updated grammar.md to move "Array typedefs" from unsupported to supported list.
- **Notes**: None.
