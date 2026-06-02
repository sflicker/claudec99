# Stage 86 Kickoff: Multidimensional Array Support

## Summary

Stage 86 adds support for multidimensional arrays in both local variables and struct/union members. The implementation enables arbitrary nesting up to 8 dimensions, with support for indexing (`A[i][j]`), sizeof operations on multidimensional type names (`sizeof(int[N][M])`), struct member access, and pointer-to-struct operations.

Core behaviors:
- `int A[2][2]` creates an array of 2 arrays of 2 ints
- `A[0][1]` uses chained indexing with proper type decay at each step
- `sizeof(int[4][8])` returns 128 (4 * 8 * sizeof(int))
- Struct layout treats multidimensional array members as a single field with computed total size
- Address calculation scales by the element type's size at each indexing level

## Tokenizer Changes

No changes required. Array bracket tokens (`[`, `]`) are already tokenized correctly.

## Parser Changes

**ParsedDeclarator structure**:
- Add `array_dims[8]` array to store each dimension
- Add `array_dim_count` field to track the number of dimensions

**parse_declarator function**:
- Extend the bracket-parsing loop to consume all `[N]` suffixes (currently processes only one)
- Store each dimension in `array_dims` and increment `array_dim_count`

**Type construction**:
- Add `build_array_type_from_dims` helper that builds the nested array type from right-to-left
  - For `int table[4][8]`: builds `array[8] of int`, then wraps it as `array[4] of (array[8] of int)`
- Update all call sites that construct single-dimension arrays with `type_array(base_type, d.array_length)` to instead call the new builder with the dimensions array
- Remove direct array handling at type construction time; delegate to builder

**parse_type_name function**:
- Extend to support `[N]` suffixes for `sizeof(int[N][M])` syntax
- Store the `full_type` on `TYPE_ARRAY` results for downstream sizeof handling

**parse_unary function** (sizeof path):
- When `parse_type_name` returns a `TYPE_ARRAY`, preserve the `full_type` for use in code generation

## AST Changes

No new node types required. Multidimensional arrays are represented as nested `TYPE_ARRAY` structures in the type system. Sequential indexing is represented as nested `AST_ARRAY_INDEX` nodes.

Example: `A[i][j]` is `AST_ARRAY_INDEX(AST_ARRAY_INDEX(A, i), j)`

## Code Generation Changes

**emit_array_index_addr function** (existing subscript handler for stage 42):
- Add handling for `TYPE_ARRAY` element types during nested subscript cases
- When the element type is an array, the base address is the result of the previous indexing (in rax), not a loaded pointer
- Scale the index by the size of the element type (which may itself be an array)

**AST_SIZEOF_TYPE handler**:
- Add `TYPE_ARRAY` to the condition for using `full_type->size` instead of base_type->size
- Ensures `sizeof(int[4][8])` correctly returns the total size

**AST_SIZEOF_EXPR handler**:
- Add `get_subscript_element_type` helper to determine the element type at each indexing level
- Add case for `AST_ARRAY_INDEX` children to compute the size of the indexed result
- Example: `sizeof(table[0])` where `table` is `array[4][8] of int` returns 32

**AST_ARRAY_INDEX rvalue handler**:
- When the element type is `TYPE_ARRAY`, decay the array to a pointer without loading
- Set `result_type` to `TYPE_POINTER` instead of the array type
- The actual pointer calculation happens in the address computation, and the decay prevents double dereferencing

## Test Plan

**Valid tests (10)**:
1. Basic 1x1 array: declare and index
2. 2x2 integer array with multiple assignments
3. Struct member (4x8 array), index and return value
4. sizeof with type names: `sizeof(int[1])`, `sizeof(int[2][2])`, `sizeof(int[4][8])`
5. Pointer-to-struct access: `p->table[i][j]`
6. Dynamic indices: loop variables as subscripts
7. 2D char array with assignment and arithmetic
8. Struct layout with preceding field (char before 4x8 array)
9. 3D array declaration and indexing
10. sizeof return value: `return sizeof(s.table[0])`

**Integration tests (2)**:
- Printf-based sizeof tests for int[N] and int[N][M] and int[N][M][P] dimension combinations

**Invalid tests (3)**:
- Too many subscripts: `s.table[0][0][0]` where table is 2D
- Non-integer array dimension: `table[4][n]` where n is a variable
- Missing inner dimension: `table[4][]` (not supported)

