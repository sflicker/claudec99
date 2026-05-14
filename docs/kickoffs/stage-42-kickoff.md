# Stage 42 Kickoff

## Spec Summary

Stage 42 adds comprehensive tests and type-enforcement for arrays of pointers. The stage covers:

- Basic arrays of pointers (`int *`, `char *`, struct pointers)
- Mutation through pointer array elements
- Array element reassignment
- Pointer arithmetic through array elements
- Arrays of `void *` with compatible type conversion
- Array decay to pointer-to-pointer when passed to functions
- Null pointer (0) assignment to pointer arrays
- Invalid patterns: nonzero integer to pointer, incompatible pointer type assignment

No grammar changes required.

## Tokenizer Changes

None.

## Parser Changes

**File: `src/parser.c`**

In `parse_postfix`, relax the subscript-base check to allow `AST_ARRAY_INDEX` in addition to `AST_VAR_REF` and `AST_DEREF`. This enables nested subscript patterns:
- `names[0][1]` — subscript into char pointer from array
- `items[0][0]` — subscript into pointer element from outer array

## AST Changes

None.

## Code Generation Changes

**File: `src/codegen.c`**

1. **`emit_array_index_addr` function**: Add new case for `base_node->type == AST_ARRAY_INDEX`
   - Evaluate address of inner subscript
   - Load pointer value
   - Use as pointer-base for outer index
   - Reject `void *` element (cannot index through void)

2. **Array-element assignment path** (approximately line 1282): When assigning to array element
   - After evaluating RHS, add pointer type checks when `element->kind == TYPE_POINTER`
   - Allow: null pointer (0), compatible pointers, `void *` conversions
   - Reject: nonzero integer to pointer, incompatible pointer types via `pointer_types_assignable`

## Test Plan

### Valid Tests (15 new tests)

1. Basic `int *` array — store/dereference int pointers
2. `char *` array with string subscript — `names[0][0]` pattern
3. Mutate through pointer array — assignment through dereferenced element
4. Pointer array element reassignment — `items[1] = items[0]`
5. Reassign pointer array element — change element multiple times
6. `char *` array subscript — `names[0][1] + names[1][1]`
7. Pointer arithmetic through array element — `names[0] + 2`
8. Array of struct pointers — access struct fields through arrow operator
9. Mutate struct through pointer array — modify struct fields via array elements
10. Typedef pointer array — use typedef'd pointer type
11. Typedef struct pointer array — linked list via array
12. Pointer-to-pointer from array decay — pass array to function expecting `int **`
13. Arrays of `void *` — store int pointers in void array
14. Store different object pointer types in void array — mixed pointer types
15. Null pointer in void array — assign 0 to pointer array elements

### Invalid Tests (4 new tests)

1. Assign nonzero integer to pointer array element — reject `items[0] = 123`
2. Assign wrong pointer type — reject `items[0] = &x` when items is `char *[]`
3. Cannot dereference `void *` element directly — reject `*items[0]` when `items[0]` is void
4. Cannot index through `void *` — reject `items[0][0]` when `items[0]` is void

## Spec Issues to Fix During Implementation

The spec contains several typos in test code that must be corrected:

1. "Mutate through pointer array" test (line 42) — missing `int main() {` wrapper
2. "Mutate through pointer array" test — missing closing `}`
3. "Mutate struct through pointer array" struct def (line 142–143) — missing `;` after closing brace
4. "typedef pointer array" test (line 170–176) — variable name mismatch: declared as `Items` but used as `items`
5. "Cannot dereference void * element directly" test (line 314) — uses undeclared variable `x`, should be `int x;`

## Implementation Order

1. **Parser changes** — enable nested subscripts first
2. **Code generation changes** — nested array indexing, pointer type validation
3. **Testing** — add all valid and invalid test cases
4. **Spec corrections** — fix typos in test code during test implementation

## Key Decisions

- Pointer type compatibility checking uses existing `pointer_types_assignable` function
- `void *` is treated as compatible with any object pointer for assignment (bidirectional)
- Null pointer (0) is always assignable to any pointer type
- Nonzero integers are never assignable to pointer types
- Array subscripts require non-void pointer base (reject `void *` in both deref and subscript contexts)
