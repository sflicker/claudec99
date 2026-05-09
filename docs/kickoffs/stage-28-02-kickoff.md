# Stage 28-02 Kickoff: Pointer Typedef Support

## Summary of Spec

Extend typedef to support pointer types. Currently, typedef only handles scalar type declarations like `typedef int I;`. This stage adds support for typedef declarations that declare pointer types, such as `typedef int *IntPtr;` or `typedef char *String;`. The typedef name can then be used as a complete pointer type in variable declarations.

## Required Tokenizer Changes

None — no new tokens are needed. The existing tokenizer handles all syntax for pointer declarations.

## Required Parser Changes

All changes are in `src/parser.c` and `include/parser.h`:

1. **TypedefEntry structure**: Add a `struct Type *full_type` field to store the complete type (including pointer qualifiers).

2. **parser_register_typedef function**: Update to accept and store a `Type *full_type` parameter in addition to the existing typedef name and base kind.

3. **parse_type_specifier** (typedef identifier case): When a typedef name is encountered and its entry has a non-NULL `full_type`, return that `full_type` directly instead of constructing a scalar type from the kind field.

4. **Block-scope typedef parsing**: Remove the existing rejection of declarators with `pointer_count > 0`. Compute the full type chain (base_type + pointer_count stars) and register with the `full_type` parameter.

5. **File-scope typedef parsing**: Apply the same changes as block-scope typedef parsing.

6. **Local declaration type assignment**: When assigning `full_type` to a declaration, set `decl->full_type = full_type` even when `d.pointer_count == 0` if the base kind is `TYPE_POINTER` (using a pointer typedef without additional stars).

7. **Global declaration type assignment**: Apply the same fix as local declaration type assignment.

8. **Multi-declarator list processing** (both local and global scopes): Apply the same `full_type` assignment logic.

## Required AST Changes

None — the existing `AST_TYPEDEF_DECL` and `AST_DECLARATION` nodes, with their existing `full_type` field, are sufficient to represent pointer typedefs.

## Required Code Generation Changes

None — existing code generation correctly handles `TYPE_POINTER` declarations when `full_type` is set.

## Test Plan

Four tests (from spec):

1. **Basic int pointer typedef**: `typedef int *IntPtr;` used to declare a local pointer variable, initialized with an address, dereferenced for return value.

2. **Char pointer typedef**: `typedef char *String;` used to declare a local pointer variable, initialized with an address, dereferenced for return value.

3. **Chained typedef**: `typedef int I; typedef I *IPtr;` where the second typedef uses a typedef'd type. Declares a local pointer and dereferences it.

4. **Multi-declarator file-scope globals**: `typedef int *P; P a,b;` at file scope, with assignment and dereference. **Note**: Test 4 in the spec contains `a = &9;` which attempts to take the address of an integer literal — the parser rejects this with "address-of requires an lvalue". This appears to be a spec typo for `a = &x;`.
