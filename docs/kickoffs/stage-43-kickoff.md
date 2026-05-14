# Stage 43 Kickoff — File-scope String and Array Initializers

## Summary

Stage 43 implements file-scope static initialization and data emission for:
1. `char s[] = "abc"` — char array from string literal
2. `char *s = "abc"` — char pointer from string literal  
3. `char *names[] = {"if", "else", "while"}` — array of char pointers from string literals
4. `int values[] = {10,20,30}` — integer array from constant expressions

Out of scope: struct aggregate initializers, nested aggregates, designated initializers.

### Baseline
534 tests passing before this stage.

## Tokenizer Changes

None required — all needed tokens already exist.

## Parser Changes

One fix required: currently `[]` (omitted array size) with a brace-initializer list is rejected. Allow size inference from the brace list element count, enabling `int values[] = {1,2,3}` and `char *names[] = {"a","b"}` syntax.

## AST Changes

None — `AST_INITIALIZER_LIST` and `AST_STRING_LITERAL` already exist.

## Code Generation Changes

1. **`GlobalVar` struct**: add `ASTNode *init_node` field to carry complex initializers.

2. **`codegen_add_global`**: handle three new initializer cases:
   - `TYPE_ARRAY + AST_STRING_LITERAL` child → mark initialized, store init_node
   - `TYPE_POINTER + AST_STRING_LITERAL` child → add to string pool, record label
   - `TYPE_ARRAY + AST_INITIALIZER_LIST` child → mark initialized, store init_node

3. **`codegen_emit_data`**: emit the new cases:
   - Char array from string literal: `db <bytes>, 0` with zero-pad to array length
   - Pointer from string literal: `dq Lstr<N>`
   - Integer/pointer array from initializer list: one `dd`/`dq` per element

## Test Plan

4 new tests:

1. `test_file_scope_char_array_string_init__0.c` + `.expected`
   - Source: `char s[] = "abc"; puts(s);` → exit 0, output "abc\n"

2. `test_file_scope_char_ptr_string_init__0.c` + `.expected`
   - Source: `char *s = "abc"; puts(s);` → exit 0, output "abc\n"

3. `test_file_scope_ptr_array_string_init__0.c` + `.expected`
   - Source: `char *names[] = {"if","else","while"}; puts(names[1]);` → exit 0, output "else\n"

4. `test_file_scope_int_array_init__60.c`
   - Source: `int values[] = {10,20,30}; return values[0]+values[1]+values[2];` → exit 60

---

Ready to begin implementation.
