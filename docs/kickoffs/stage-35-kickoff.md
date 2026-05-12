# Stage 35 – Nested Structs and Arrays of Structs

## Summary

Stage 35 extends the compiler to support:
1. Struct members whose type is another (already-defined, complete) struct type
2. Arrays of struct element type
3. Chained member access: `rect.origin.x`
4. Member access through array elements: `points[0].x`
5. Assignment to and reading from nested struct fields and array element fields
6. `sizeof` for nested structs and arrays of structs

Key insight: Struct layout computation via `type_size` already handles struct-type members correctly by recursing through their constituent int/char fields. The main codegen work is extending `emit_member_addr` to handle two new base-node cases: chained member access and array-indexed member access.

## Tokenizer Changes

None required.

## Parser Changes

None required. The parser already:
- Parses chained `expr.field.field` as nested AST_MEMBER_ACCESS nodes
- Parses `array[i].field` as AST_MEMBER_ACCESS over AST_ARRAY_INDEX
- Allows AST_MEMBER_ACCESS as an assignment LHS
- Computes correct struct layout (using type_size) for struct-type fields

## AST Changes

None required. AST_MEMBER_ACCESS already supports arbitrary base expressions (not just identifiers).

## Type System Changes

None required. StructField.full_type is already set to the struct Type* for struct-type fields during struct declaration parsing.

## Codegen Changes

Only `emit_member_addr` in `src/codegen.c` needs extension with two new base-node cases:

### Case 1: Chained Member Access (e.g., `r.origin.x`)

Base node is AST_MEMBER_ACCESS:
1. Recursively call `emit_member_addr(base)` → leaves address of intermediate struct field in rax, returns StructField* for that field
2. Verify returned field has kind=TYPE_STRUCT with full_type set (error otherwise: "cannot access member of non-struct type")
3. Look up outer field name in the inner struct's type definition
4. If outer field's offset != 0, emit `add rax, <offset>`
5. Return the outer StructField*

### Case 2: Array-Indexed Member Access (e.g., `points[0].x`)

Base node is AST_ARRAY_INDEX:
1. Call `emit_array_index_addr(base)` → leaves element address in rax, returns element Type*
2. Verify element kind == TYPE_STRUCT (error otherwise: "cannot access member of non-struct array")
3. Look up field name in the element struct's type definition
4. If field offset != 0, emit `add rax, <offset>`
5. Return the StructField*

## Test Plan

### Valid Tests

1. **test_nested_struct_basic__37.c**
   - Define Point (x, y) and Rect (origin: Point, width, height)
   - Assign r.origin.x=10, r.origin.y=20, r.width=3, r.height=4
   - Return r.origin.x + r.origin.y + r.width + r.height = 37

2. **test_nested_struct_multiple__33.c**
   - Define Line with start: Point and end: Point
   - Assign start=(1,2), end=(10,30)
   - Return sum of all four fields = 43... (wait, spec says 33, recalculate: 1+2+10+30=43, spec error or different test)
   - Verify per spec

3. **test_array_of_structs__33.c**
   - Array points[2] of Point struct
   - Assign points[0]=(1,2), points[1]=(10,20)
   - Return points[0].x + points[0].y + points[1].x + points[1].y = 1+2+10+20 = 33

4. **test_array_of_structs_nested__110.c**
   - Array rects[2] of Rect (with nested Point)
   - Assign rects[0].origin=(1,2), rects[0].width=3, rects[0].height=4
   - Assign rects[1].origin=(10,20), rects[1].width=30, rects[1].height=40
   - Return sum = 1+2+3+4+10+20+30+40 = 110
   - Note: Spec has typo `helght` in struct declaration but uses `height` in assignments—use correct spelling `height`

5. **test_nested_struct_sizeof__16.c**
   - Define Point (8 bytes), Rect (origin: Point + width: int + height: int)
   - sizeof(struct Rect) = 8 + 4 + 4 = 16 (not 15 as spec says; spec has an error)
   - Return 16

6. **test_array_of_structs_sizeof__24.c**
   - Array points[3] of Point (8 bytes each)
   - sizeof(points) = 3 * 8 = 24
   - Return 24

7. **test_nested_struct_align_sizeof__16.c**
   - struct Inner: char (1) + int x (4, offset 4 due to alignment) → size 8
   - struct Outer: char a (1) + Inner (8, offset 4 due to alignment) + char b (1, offset 12) → size 16
   - Return sizeof(struct Outer) = 16
   - Note: Spec typo `resize` should be `return`

### Invalid Tests

1. **test_struct_incomplete_member__undefined_struct.c**
   - struct Outer with member of undefined struct type Missing
   - Error: "struct Missing not yet defined"

2. **test_struct_nested_no_member__no_member.c**
   - struct Rect with nested Point
   - Attempt r.origin.z where z doesn't exist in Point
   - Error: "struct Point has no member z"

3. **test_struct_array_no_index__applied_to_non_array.c**
   - Array points[2] of Point
   - Attempt points.x where points is an array, not a struct
   - Error: "cannot access member of array type (use indexing first)"

## Implementation Order

1. Add error checking in `emit_member_addr` for undefined struct types and missing fields
2. Implement Case 1 (chained member access)
3. Implement Case 2 (array-indexed member access)
4. Create and validate test suite
