# Stage 33 Kickoff: Struct Assignment

## Spec Summary

Stage 33 adds whole-struct copy support. Two forms are needed:

1. **Assignment**: `d = c` — copies struct fields byte-by-byte
2. **Declaration initializer**: `struct Point d = c` — copy from another struct variable at declaration time

Both are rejected when the source and destination struct types differ. Same struct type is determined by checking if source and destination have the same `Type*` pointer (same struct definition).

### Example
```c
struct Point {
    int x;
    int y;
};

int main() {
    struct Point c = { 3, 4 };
    struct Point d = c;        // declaration initializer form
    d = c;                     // assignment form
    return d.x + d.y;         // expect 7
}
```

## Required Tokenizer Changes

None. All tokens (identifiers, `=`) already exist.

## Required Parser Changes

None. The parser already creates:
- `AST_ASSIGNMENT` with LHS identifier and RHS `AST_VAR_REF` for `d = c`
- `AST_DECLARATION` with a non-initializer-list child for `struct Point d = c`

## Required AST Changes

None.

## Required Code Generation Changes

Two changes in `src/codegen.c`:

### 1. Struct assignment (`d = c`)

In the `AST_ASSIGNMENT` local var block, before calling `codegen_expression` on the RHS:

- Check if `lv->kind == TYPE_STRUCT`
- Look up the RHS (must be `AST_VAR_REF` for a struct variable of the same type)
- Emit byte-by-byte copy using:
  - `movzx eax, byte [rbp - src_off - b]` 
  - `mov [rbp - dst_off - b], al` 
  - for `b` in `[0, size)`
- Type check: reject if `source full_type != dest full_type` (must have same `Type*` pointer)

### 2. Declaration initializer (`struct Point d = c`)

In the struct local declaration block (after brace-initializer handling):

- When a child exists and is NOT an `AST_INITIALIZER_LIST`
- Check that the child is an `AST_VAR_REF` for a struct of the same type
- Emit the same byte-by-byte copy as above

## Test Plan

### Valid tests

- **test/valid/test_struct_assign_copy__7.c**: Simple assignment `d = c`, return `d.x + d.y` → expect 7
- **test/valid/test_struct_decl_init_copy__7.c**: Declaration initializer `struct Point d = c`, return `d.x + d.y` → expect 7

### Invalid test

- **test/invalid/test_struct_assign_type_mismatch__incompatible_struct_types.c**: Type mismatch `q = p` where q is `struct Other` and p is `struct Point` → expect error containing "incompatible struct types"

## Implementation Order

1. Implement struct assignment in `codegen_expression` (AST_ASSIGNMENT case)
2. Implement declaration initializer in struct declaration codegen
3. Create valid test cases
4. Create invalid test case for type checking
5. Verify all tests pass
