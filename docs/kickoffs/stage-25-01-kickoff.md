# Stage 25-01 Kickoff: Function Pointer Declarations

## Spec Summary

Stage 25-01 adds support for **declaring** (but not calling or assigning) function-pointer variables, parameters, and file-scope objects. The stage introduces a new `TYPE_FUNCTION` kind to the type system to represent function types and distinguishes between:

- Function declarations: `int f(int);` — declares a function
- Function pointer declarations: `int (*fp)(int);` — declares a pointer to a function

Supported forms include local function pointers, file-scope function pointers, static function pointers, extern function pointer declarations, and function pointer parameters. Type compatibility checks based on return type, parameter count, and parameter types are required. Out of scope: function pointer assignments, indirect calls, function-returning-pointer declarations, and pointer-to-function-pointer declarations.

**Spec issues noted:**
- Line 4: Title typo "declaraing" should be "declaring"
- Line 9: Goal example `int (op)(int a, int b)` is missing the `*` — should be `int (*op)(int a, int b)` to be a function pointer. The implementation correctly treats the no-`*` parenthesized form as a function declaration (valid C99 behavior).
- Line 163: Last valid test has typo `return main() {` — should be `int main() {`

## Required Changes

### Type System Changes
- **include/type.h**
  - Add `TYPE_FUNCTION` to `TypeKind` enum
  - Add `param_count` and `params[]` fields to `Type` struct to track function signature
  - Add `type_function()` constructor declaration

- **src/type.c**
  - Implement `type_function()` constructor
  - Update `type_kind_name()` to handle `TYPE_FUNCTION`
  - Update `type_is_integer()` to exclude function types

### Parser Changes
- **include/parser.h**
  - Add `full_type` field to `GlobalObjSig` struct to store complete type information

- **src/parser.c**
  - Extend `ParsedDeclarator` with function-pointer detection fields: `is_func_pointer`, `fp_outer_stars`, `fp_inner_stars`
  - Revamp `parse_declarator()` to distinguish between:
    - Function declarations: `f(int)` 
    - Function pointer declarations: `(*fp)(int)` with optional outer stars
    - Regular pointers: `*p`
  - Add `parse_func_ptr_param_types()` helper to parse function pointer parameter lists
  - Update `parse_parameter_declaration()` to handle function pointer parameters
  - Update `parse_statement()` and `parse_external_declaration()` to accept function pointer declarations
  - Update `parser_register_global()` to perform type compatibility checks between extern and definition, accounting for full type information

### Code Generation Changes
- **src/codegen.c**
  - Add `TYPE_FUNCTION` case to `type_kind_bytes()` (return pointer-size)
  - No other changes needed for declaration-only support

### Test Changes
- Delete `test_invalid_119` and `test_invalid_121` (now valid function pointer forms)
- Add 7 valid tests covering: local function pointers, multi-parameter function pointers, file-scope function pointers, static function pointers, extern followed by definition, function pointer parameters, function pointers returning pointers
- Add 5 invalid tests covering: parameter type mismatch, return type mismatch, duplicate static definitions, function pointer calls (unsupported), function pointer assignments (unsupported)

## Implementation Order

1. **Type system** — implement `type_function()` and type compatibility checks
2. **Parser** — implement declarator parsing, global object registration, and parameter handling
3. **Code generation** — add minimal `TYPE_FUNCTION` support
4. **Tests** — update test suite

## Ambiguities and Decisions

1. **Parenthesized declarators without `*`**: The spec goal example `int (op)(int a, int b)` lacks the `*` pointer marker. This is a function declaration with redundant parentheses (valid C99), not a function pointer. Implementation treats this correctly as a function declaration, consistent with C99 semantics.

2. **Type struct expansion**: Function types require storing parameter count and parameter types in the `Type` struct. Need to decide on maximum parameter count to avoid dynamic allocation.

3. **Outer vs. inner stars**: Function pointers can have stars both before the `*` (return type pointers) and in the parameter list (parameter pointers). The implementation must parse and track both independently.

4. **Extern followed by definition**: An `extern` declaration of a function pointer can be followed by an actual definition. Type compatibility checks must verify parameter types match exactly.
