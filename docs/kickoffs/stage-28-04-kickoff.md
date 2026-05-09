# Stage 28-04 Kickoff: Array Typedefs

## Summary of the Spec

Stage 28-04 extends typedef support to array types, building on the pointer and function-pointer typedef work completed in Stage 28-03. Previously, only scalar, pointer, and function-pointer typedefs were supported. The stage adds support for declarations like `typedef int A[4];`, allowing type names to represent fixed-size arrays. Type-qualified arrays declared via typedefs can be:
- Used in variable declarations: `A a;`
- Indexed: `a[0]`, `a[1]`, etc.
- Queried with sizeof: `sizeof(a)`
- Referenced through pointers: `A *p = &a;` and `(*p)[0]`

Five test cases validate the feature:
- Basic array typedef with element storage and retrieval
- Multi-declarator syntax with typedef arrays
- sizeof operator on typedef array variables (both int and char arrays)
- Pointer-to-typedef-array dereferencing with array indexing

## Required Tokenizer Changes

None. No new keywords or token types are required. The array declaration syntax `[size]` already exists and is lexically handled by the current tokenizer.

## Required Parser Changes

### Block-Scope Typedef Declarations
In `src/parser.c`, the `handle_block_scope_typedef` function currently rejects array types via a blanket check of `d.is_array`. This logic must be removed and replaced with:
- Register a TYPE_ARRAY typedef with `full_type = type_array(base_type, d.array_length)`
- Store the array type reference in the typedef symbol table entry

### File-Scope Typedef Declarations
Apply the same fix to `handle_file_scope_typedef` to support file-scope array typedefs.

### Block-Scope Variable Declarations
Add an `else if (base_kind == TYPE_ARRAY)` branch in the block-scope variable declaration handler:
- When a variable is declared using a typedef'd array type name as the base_type
- Set `decl->full_type = full_type` to carry the array Type* chain into the AST

### Multi-Declarator Lists
Apply the same fix to each declarator in a multi-declarator list (e.g., `A a, b;`):
- Each declarator within the list must correctly resolve to the full array type
- This ensures all variables in the list inherit the typedef'd array type

## Required AST Changes

None. The existing AST_DECLARATION and AST_TYPEDEF_DECL nodes are sufficient to represent array typedefs. The full_type field in AST_DECLARATION nodes already carries Type* chains, including array types. No new node types or structural changes are needed.

## Required Code Generation Changes

### Array Index Address Emission
In `src/codegen.c`, the `emit_array_index_addr` function currently only handles direct array base nodes. Add handling for pointer dereference:
- Detect `base_node->type == AST_DEREF` cases
- Load the pointer value from the dereferenced expression (which represents the array base address)
- Use the array's element type for correct scaling in index calculation
- This supports patterns like `(*p)[0]` where `p` is a pointer to a typedef'd array

## Test Plan

Five new test files in `test/valid/`:

1. **test_typedef_array_basic__10.c**
   - Declares `typedef int A[4]` with variable `A a`
   - Stores values 1, 2, 3, 4 in array elements
   - Returns sum (expect 10)

2. **test_typedef_array_multi__8.c**
   - Uses multi-declarator syntax: `A a, b;`
   - Indexes both arrays independently
   - Returns sum of elements (expect 8)

3. **test_typedef_array_sizeof_int__16.c**
   - Declares `typedef int A[4]` with variable `A a`
   - Calls `sizeof(a)` on the typedef array
   - Returns size (expect 16 bytes for 4 int elements)

4. **test_typedef_array_sizeof_char__8.c**
   - Declares `typedef char Name[8]` with variable `Name n`
   - Calls `sizeof(n)` on the typedef array
   - Returns size (expect 8 bytes for 8 char elements)

5. **test_typedef_array_ptr__7.c**
   - Declares array typedef and pointer: `A *p = &a;`
   - Uses pointer-to-array dereference and indexing: `(*p)[0] = 7`
   - Returns value through original array reference
   - Returns value (expect 7)

## Implementation Order

1. Modify block-scope typedef handler to accept array declarators
2. Modify file-scope typedef handler to accept array declarators
3. Modify block-scope variable declaration to handle array typedef bases
4. Modify multi-declarator list handling for array typedef bases
5. Add emit_array_index_addr support for pointer dereference patterns
6. Implement and validate all five test cases

## Known Decisions

- Array typedefs store the full array type in the typedef registry, not just the element type
- Multi-declarator syntax relies on parser-level type resolution, not symbol table expansion
- Pointer-to-array indexing is codegen-level, handled by load and scale operations on dereferenced pointer values
