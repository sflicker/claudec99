# Stage 137 Kickoff: Functions Returning Function Pointers

## Spec Summary

CC99-010 identifies that the parser rejects valid C99 function declarations where a function returns a pointer to a function, e.g., `int (*get_adder())(int)`. The spec clarifies that while returning a function-type directly is invalid (`int f()(int)`), returning a pointer-to-function is valid and currently unsupported.

Current error:
```
functions returning function pointers are not supported
```

Expected behavior: parse and codegen the returned function pointer as a normal pointer value that can be assigned, called indirectly, or passed.

## Tokenizer Changes

None required.

## Parser Changes

**Root cause:** In `parse_declarator`, when processing a parenthesized declarator `(*name)`, the parser sees `TOKEN_LPAREN` after the inner identifier and rejects it. The fix detects `inner_stars > 0` and parses both the function's own parameter list and the returned function pointer's parameter list inline.

**ParsedDeclarator struct:**
- Add: `is_func_returning_fp` (bool)
- Add: `own_param_types[]` (Type array)
- Add: `own_param_count` (int)
- Add: `own_is_no_prototype` (bool)

**parse_declarator function:**
- Replace the "not supported" error with full `(*name())(params)` parsing logic
- Populate the new struct fields when `inner_stars > 0` and function declarator is detected

**parse_external_declaration function:**
- Add branch to handle `is_func_returning_fp` case
- Fix func->full_type for typedef pointer return types: change condition from `d.pointer_count > 0` to `return_kind == TYPE_POINTER`

## AST Changes

No new AST node types required. Existing `AST_FUNCTION_CALL` and `AST_INDIRECT_CALL` paths handle function pointer calls.

## Code Generation Changes

None required. The returned function pointer is a normal pointer value; existing codegen handles indirect function calls.

## Test Plan

**Valid tests to add (test/valid/functions/):**
- Function definition returning function pointer via `int (*f())(int)` syntax
- Assignment of returned function pointer to `int (*p)(int)` variable
- Direct call through returned pointer, e.g., `f()(11)`
- Equivalent declaration using typedef for the function-pointer type

**Invalid tests:**
- Move test/invalid/legacy/test_invalid_122 to valid suite (now valid with this stage)
- Add new invalid test: function returning function type directly `int f()(int)` (inner_stars == 0 case)

## Implementation Order

1. Update ParsedDeclarator struct
2. Modify parse_declarator to handle `(*name())(params)` parsing
3. Update parse_external_declaration to set is_func_returning_fp and fix return type handling
4. Bump version to 13700000
5. Add valid test cases
6. Move/reclassify invalid test case
7. Verify self-compilation

## Decisions

- The function pointer's own parameters and the returned pointer's parameters are both parsed inline within parse_declarator, avoiding separate data structures
- typedef'd pointer return types use the same mechanism: detect `return_kind == TYPE_POINTER` rather than `d.pointer_count > 0`
