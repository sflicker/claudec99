# Stage 28-03 Kickoff: Function Pointer typedef

## Spec Summary

Extend typedef to support function pointer types (e.g., `typedef int (*BinaryFn)(int, int);`). This allows aliases for function pointer types to be used in variable declarations and parameters, enabling cleaner code when working with callbacks and function pointers.

### Core requirement
Parse function pointer typedefs like `typedef int (*Name)(param_types);` and register them as typedef aliases that resolve to the underlying function pointer type.

### Test cases
1. **Basic function pointer typedef**: declare and use a function pointer typedef in initialization
2. **Assignment after declaration**: declare typedef and then assign after declaration, with dereference call syntax
3. **Multi-declarator**: declare multiple variables of the same function pointer typedef

## Derived stage values

| Item | Value |
|------|-------|
| Affected language feature | typedef, function pointers |
| Scope | Parser changes only |
| Test count baseline | 442 valid tests (stage 28-02) |
| Test count expected | 445 valid tests (+3 new function pointer typedef tests) |
| Total test count expected | 719 total (+3 valid, no invalid/print tests affected) |

## Required changes

### Tokenizer
- **No changes needed**. All required tokens already exist (parentheses, asterisk, typedef keyword, parameter lists).

### Parser (`src/parser.c`)
Two locations require changes:

1. **Block-scope typedef** (~line 1350)
   - Remove the rejection guard that checks `d.is_func_pointer`
   - Add a new branch that detects function pointer declarators and calls `build_fp_type(base_type, &d)` to build the function pointer type
   - Register the typedef with `kind=TYPE_POINTER` and the function pointer full_type

2. **File-scope typedef** (~line 1779)
   - Same changes as block-scope: remove `is_func_pointer` rejection, add branch to build and register function pointer typedefs

The logic mirrors existing code paths for scalar and pointer typedefs. The key is that `build_fp_type()` already exists and produces a proper TYPE_POINTER type wrapping the function signature.

### AST
- **No changes needed**. `AST_TYPEDEF_DECL` is sufficient to represent function pointer typedefs. The type information is stored in the registered typedef entry.

### Code generation
- **No changes needed**. All machinery for handling TYPE_POINTER→TYPE_FUNCTION already exists from stages 25-26 (function pointer variables). Function pointer typedef resolution follows the same type chain as non-typedef function pointer variables.

## Test plan

Create three new valid test files in `test/valid/`:

1. `test_typedef_func_ptr_basic__5.c`
   - Basic typedef with initialization
   - Expected exit: 5

2. `test_typedef_func_ptr_assign__5.c`
   - Typedef with assignment after declaration
   - Uses dereference call syntax `(*f)(args)`
   - Expected exit: 5

3. `test_typedef_func_ptr_multi__7.c`
   - Multi-declarator typedef (two variables of same function pointer typedef)
   - Expected exit: 7

No invalid/print tests needed for this stage.

## Documentation changes

- `docs/grammar.md`: Update typedef notes (line ~176) to document support for function pointer typedefs
- `README.md`: Update stage reference from 28-02 to 28-03; update test count summary to 445 valid, 719 total

## Implementation order

1. Parse and test block-scope function pointer typedef parsing
2. Parse and test file-scope function pointer typedef parsing
3. Verify codegen integration (should require no changes)
4. Run full test suite
5. Update grammar.md and README.md
6. Commit and generate milestone summary

## Key decisions and ambiguities

- **Registration strategy**: Function pointer typedefs are registered with `kind=TYPE_POINTER` and the full function pointer type, following the same registration pattern as pointer typedefs. This allows `typeof` resolution to work consistently.
- **Multi-declarator support**: Function pointer typedefs support comma-separated declarators (e.g., `typedef int (*Fn)(int), (*Fn2)(int);`), though the common pattern is single declarators.
- **Scope tracking**: Block-scope function pointer typedefs shadow file-scope ones, following the same shadowing rules as all typedefs.
