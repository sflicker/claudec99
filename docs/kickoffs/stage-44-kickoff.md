# Stage 44 Kickoff — Aggregate Initializers

## Summary

Stage 44 extends the compiler to support struct and array-of-struct initializers in all contexts:

1. **File-scope struct initializer**: `struct Point p = {3, 4};`
2. **File-scope array of structs**: `struct Point points[] = {{1,2},{10,20},{100,200}};`
3. **Local struct initializer**: `struct Point p = {10,20};` (currently compiles but gives wrong results — bug fix required)
4. **Local nested struct initializer**: `struct Rect r = {{1,2}, 3, 4};` (nested structs)
5. **Local array of structs**: `struct Point points[2] = {{1,2},{10,20}};`
6. **Struct field with char***: initialized from string literal at file scope

Out of scope: designated initializers, partial initializers with explicit field names.

### Baseline

538 tests passing before this stage (534 valid + 4 from stage 43).

## Tokenizer Changes

None required — all needed tokens already exist.

## Parser Changes

1. **File-scope non-array non-function declarations**: When `decl_type == TYPE_STRUCT` and the next token is `{`, use `parse_initializer` (brace list) instead of `parse_primary` (which rejects `{`).

2. **Array initializer validation**: Add too-many-elements check. Currently `int a[2] = {1,2,3}` is silently accepted; should be rejected.

## AST Changes

None — `AST_INITIALIZER_LIST` already exists and is already produced by the recursive `parse_initializer`.

## Code Generation Changes

1. **New local helper `emit_local_struct_init`**:
   - Signature: `emit_local_struct_init(CodeGen *cg, Type *st, int base_offset, ASTNode *list)`
   - Recursively handles nested struct fields
   - Handles pointer fields: type-checks string literals and rejects non-pointer initializers
   - Rejects string literals for non-pointer fields
   - Rejects non-null integers for pointer fields
   - Zero-fills fields beyond the initializer list

2. **New global helper `emit_global_struct`**:
   - Signature: `emit_global_struct(CodeGen *cg, Type *st, ASTNode *list)`
   - Emits `.data` entries for each struct field recursively
   - String-literal pointer fields: add to string pool, emit `dq Lstr<N>`
   - Scalar fields: emit `db`/`dw`/`dd`/`dq` with value
   - Zero-fills missing fields

3. **`codegen_add_global`**: Add case for `TYPE_STRUCT + AST_INITIALIZER_LIST` → set `init_node`, `is_initialized = 1`

4. **`codegen_emit_data`**: 
   - Add `TYPE_STRUCT` case using `emit_global_struct`
   - Update `TYPE_ARRAY` case to handle `AST_INITIALIZER_LIST` elements (struct-element arrays)
   - Add too-many-elements validation

5. **Local struct init loop**: Replace inline `codegen_expression` on struct sub-fields with call to `emit_local_struct_init`

6. **Local array init loop**: Add too-many-elements check; handle struct elements with `emit_local_struct_init`

## Test Plan

### Valid Tests (8 new)

1. `test_file_scope_struct_init__7.c`
   - File-scope `struct Point p = {3,4}` → exit 7

2. `test_local_struct_init__30.c`
   - Local `struct Point p = {10,20}` → exit 30

3. `test_local_array_of_structs_init__33.c`
   - Local `struct Point[2]` nested brace init → exit 33

4. `test_file_scope_array_of_structs_init__111.c`
   - File-scope `struct Point[3]` → exit 111

5. `test_local_nested_struct_init__10.c`
   - Nested struct `struct Rect r = {{1,2},3,4}` → exit 10

6. `test_file_scope_string_field_table__2.c` + `.expected`
   - Keywords table with char* field → exit 2, output "if\n"

7. `test_file_scope_string_field_access__4.c` + `.expected`
   - Array of struct with string fields → exit 4, output "return\n"

8. `test_puts_string_from_table__0.c` + `.expected`
   - Call `puts` on string field from table → exit 0, output "if\n"

### Invalid Tests (4 new)

1. `test_struct_init_too_many_file_scope__too_many_initializers`
   - File-scope struct with extra initializers → compile error

2. `test_struct_init_too_many_local__too_many_initializers`
   - Local struct with extra initializers → compile error

3. `test_struct_init_string_for_int_field__incompatible_field_initializer`
   - String literal for int field → compile error

4. `test_struct_init_int_for_ptr_field__incompatible_field_initializer`
   - Integer literal for pointer field → compile error

### Note on Spec

The spec includes a typo in the "compile table style initializer" test: `return keywords[3].value;` should be `return keywords[3].kind;` (the struct `TokenMapEntry` has a field named `kind`, not `value`). Tests will use the correct field name.

---

Ready to begin implementation.
