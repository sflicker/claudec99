# Stage 17-03 Kickoff: sizeof arrays

## Summary of the spec

**Stage 17-03: sizeof arrays**

Add `sizeof` support for declared array variables. When `sizeof(A)` is applied to a variable declared as an array (e.g., `int A[10]`), it should produce the compile-time constant equal to the total byte size of the array (element_size × num_elements), with no runtime code generated for the array expression.

Key requirement: Arrays must not be decayed to pointers before `sizeof` sees them, ensuring `sizeof` evaluates the array's true size, not the size of a pointer.

## Required tokenizer changes

None. The `sizeof` keyword and array syntax already exist in the tokenizer.

## Required parser changes

None. The parser already correctly parses `sizeof(A)` as an `AST_SIZEOF_EXPR` node with an `AST_VAR_REF` child. The existing parse tree structure is appropriate for this stage.

## Required AST changes

None. Existing node types (`AST_SIZEOF_EXPR`, `AST_VAR_REF`) and type structures are sufficient. Array type information is already preserved in the symbol table and type system.

## Required code generation changes

Modify `src/codegen.c` in the `AST_SIZEOF_EXPR` handler:

- Add an early check: if the child expression is `AST_VAR_REF` and the referenced variable has kind `TYPE_ARRAY`, emit a single `mov rax, <array_total_size>` instruction and return.
- The array's total byte count is already computed and stored in the variable's `full_type->size` field.
- Do not generate code to evaluate the array expression at runtime.

Example: For `int A[10]`, emit `mov rax, 40` (4 bytes × 10 elements).

## Test plan

### Valid test cases

1. `int A[10]` → sizeof returns 40
2. `char C[10]` → sizeof returns 10
3. `short S[10]` → sizeof returns 20
4. `long L[10]` → sizeof returns 80
5. `int *p[10]` → sizeof returns 80 (array of 10 pointers, 8 bytes each)

### Invalid test case

- `sizeof(int[10])` → Already rejected by parser with "expected ')'" error. This form is out of scope per spec.

## Out of scope

- `sizeof(type[N])` form (type name with array specifier)
- Variable-length arrays (VLAs)
- `struct`, `union`, `enum`
- Unsized arrays
- `double`, `float`, `size_t`
