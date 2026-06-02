# Milestone Summary

## Stage 86: Multidimensional Array Support - Complete

stage-86 ships full support for multidimensional arrays in declarations, indexing, member access, and sizeof expressions.

- **Tokenizer**: No changes.
- **Grammar/Parser**: Extended declarators to parse multiple `[N]` bracket suffixes (first may be empty for size inference; subsequent must be explicit). Updated `parse_declarator` and `parse_type_name` to build nested array types right-to-left via `build_array_type_from_dims()` helper (e.g., `int A[4][8]` becomes array[4] of array[8] of int). All declaration sites (struct/union members, local/global declarations, typedefs) use the new multi-dimensional builder.
- **AST/Semantics**: Added `MAX_ARRAY_DIMS=8` constant and dimension tracking to `ParsedDeclarator`. Type system correctly nests array types. Array-to-pointer decay applies at each level when needed (e.g., `table[i]` yields a pointer to array[8] of int, which decays on second subscript).
- **Codegen**: Added `get_subscript_element_type()` helper for determining element types through subscript chains. Updated `emit_array_index_addr` to handle nested array subscripts (when inner element is an array, scale by the inner array's byte size). Enhanced `sizeof` for type-names and expressions: `sizeof(int[4][8])` = 128, `sizeof(s.table[0])` = 32 (where table is array[4] of array[8] of int). Array-to-pointer decay in rvalue context (when subscript result is array type, decay to pointer).
- **Tests**: 11 new valid tests (10 multidimensional + 1 moved from invalid), 2 new invalid tests (too_many_subscripts, missing_inner_size), 2 new integration tests (sizeof_multidim_types, sizeof_multidim_3d). Removed 1 invalid test (sizeof_array_type_name_40 now valid).
- **Docs**: Updated grammar.md with `<abstract_array_declarator>` production for type-name context.
- **Notes**: Maximum dimensions capped at 8. All 1282 tests pass (802 valid, 236 invalid, 80 integration, 43 print-AST, 100 print-tokens, 21 print-asm).
