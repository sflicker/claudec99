# Stage 135 Kickoff: Type Compatibility and Composite Type Checks

## Summary of the Spec

Stage 135 fixes two C99 type compatibility bugs in cc99.

**CC99-008**: Array parameters are not compared after parameter adjustment.
- When a function is declared with `int f(int a[3])` and later with `int f(int *a)`, cc99 reports a parameter type mismatch.
- C99 §6.7.5.3p7 specifies that array parameter types adjust to pointer-to-element before comparison.
- These should be compatible declarations.

**CC99-009**: Pointer-to-array types are not supported.
- When `int (*row)[]` or `int (*row)[4]` appears as a parameter, cc99 rejects it with "pointer to array types are not supported".
- C99 permits pointers to array types and requires composite type rules for incomplete/complete bound pairs.
- These should be valid declarations.

Both bugs prevent valid C99 programs from compiling that GCC accepts.

## Required Tokenizer Changes

No tokenizer changes required. The tokenizer already handles all necessary input for this stage.

## Required Parser Changes

### ParsedDeclarator struct extension

Add three new fields to track pointer-to-array support:
- `int is_ptr_to_array` - flag indicating pointer to array form
- `int ptr_to_array_length` - the array bound (0 if unknown)
- `int ptr_to_array_has_size` - boolean for whether bound was specified

### parse_declarator function

Replace the error "pointer to array types are not supported" with parsing logic that:
1. Detects the `(*declarator)` pattern
2. Parses the array `[N]` or `[]` suffix following the close paren
3. Sets the three new ParsedDeclarator fields
4. Stores the bracket info for later type construction

### parse_parameter_declaration function

Add three adjustments:

1. **Array parameter adjustment (CC99-008)**:
   - After `parse_declarator`, if `d.is_array == 1`, convert to pointer type
   - Set `param->decl_type = TYPE_POINTER`
   - Build the adjusted type with `type_pointer(base_type)`

2. **Function parameter adjustment (CC99-008)**:
   - After `parse_declarator`, if `d.is_function == 1`, consume the attached `(params)` list
   - Convert to pointer-to-function type
   - Build the adjusted type with `type_pointer(function_type)`

3. **Pointer-to-array support (CC99-009)**:
   - After `parse_declarator`, if `d.is_ptr_to_array == 1`, build the type
   - Construct `pointer(array(base_type, N))` where N is 0 for unknown bounds
   - Store adjusted `decl_type = TYPE_POINTER` for composite type compatibility

## Required AST Changes

No AST changes required. The existing type system already represents pointer-to-array types; the parser just needs to construct them.

## Required Code Generation Changes

No code generation changes required. The existing implementation already handles `(*ptr_to_array)[idx]` subscript operations correctly.

## Test Plan

Five new test files:

1. **test/valid/functions/test_array_param_bracket_ptr_compat__9.c**
   - Array parameter with `[3]`, `*`, `[]` forms all compatible
   - Tests CC99-008

2. **test/valid/functions/test_array_param_unbound_compat__6.c**
   - Unbound array parameter `a[]` compatible with `*a`
   - Tests CC99-008

3. **test/valid/functions/test_func_param_adjust_compat__0.c**
   - Function parameter adjustment: `int cb(void)` adjusts to `int (*cb)(void)`
   - Tests CC99-008 function parameter case

4. **test/valid/pointers/test_pointer_to_array_param__13.c**
   - Pointer to known-bound array parameter
   - Pointer to unknown-bound compatible with known-bound redeclaration
   - Tests CC99-009

5. **test/invalid/pointers/test_invalid_pointer_to_array_bounds_mismatch__0.c**
   - Incompatible pointer-to-array bounds with different known sizes
   - Tests CC99-009 error detection

## Implementation Order

1. Update ParsedDeclarator struct with new fields
2. Implement parse_declarator changes to support pointer-to-array
3. Implement parse_parameter_declaration adjustments
4. Add test files
5. Update stage version to 13500000 in src/version.c
6. Commit with Co-Authored-By trailer
