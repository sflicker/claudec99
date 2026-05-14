# Stage 41 – Pointer Arithmetic Completion

## Summary of the Spec

Stage 41 completes pointer arithmetic support by implementing missing operators and validating unsupported cases. The stage targets the following cases:

- `p + n` and `p - n`: already implemented (Stage 13-04)
- `p += n` and `p -= n`: already work via parser binary-op expansion
- `p++` and `p--`: currently broken (uses 32-bit integer ops, not pointer-aware)
- `p1 - p2` (pointer difference): currently rejected, needs implementation
- `array[i]` indexing via pointer: already works
- `void *` arithmetic: already rejected (Stage 38)
- **Function pointer arithmetic**: new validation for Stage 41 (must reject)

Codegen rules:
```
p + n  => address(p) + n * sizeof(*p)
p - n  => address(p) - n * sizeof(*p)
p1 - p2 => (address(p1) - address(p2)) / sizeof(*p1)
```

## Required Tokenizer Changes

None.

## Required Parser Changes

None.

## Required AST Changes

None.

## Required Code Generation Changes

1. **Prefix/Postfix Increment/Decrement on Pointers** (`AST_PREFIX_INC_DEC` and `AST_POSTFIX_INC_DEC`)
   - Currently uses 32-bit integer increment/decrement
   - Must use 64-bit operations for pointer values
   - Must scale increment/decrement by `sizeof(*p)` for pointer types

2. **Pointer Difference** (`p1 - p2`)
   - Currently rejected with an error
   - Must compute `(address(p1) - address(p2)) / sizeof(*p1)`
   - Type check: both operands must be pointers to the same type
   - Result type: `long` (pointer difference)

3. **Function Pointer Arithmetic Rejection**
   - Add validation where `void *` arithmetic is already rejected
   - Reject `p + n`, `p - n`, `p++`, `p--`, etc. on function pointers

## Test Plan

**Valid test cases:**
- `ptr_diff_int`: basic pointer difference
- `ptr_postfix_increment`: postfix `++` on pointer
- `ptr_prefix_decrement`: prefix `--` on pointer
- `ptr_compound_assign`: `+=` and `-=` on pointers
- `struct_pointer_diff`: pointer difference with struct arrays

**Invalid test cases:**
- Update existing pointer subtraction invalid test to reflect new error message
- Add function pointer arithmetic rejection test (all operators: `+`, `-`, `++`, `--`, `+=`, `-=`)

## Notes and Decisions

1. **Spec documentation issues noted** (not action items):
   - Test 2 has typo "struct Piont" (should be "struct Point") – will use correct spelling in implementation tests
   - Test 2 line 53 has awkward dereference syntax `*(p + 1)->x` that relies on operator precedence – will use clearer `(p+1)->x` form in tests
   - Closing braces missing from spec test cases (documentation-only issue)

2. **Pointer difference result type**: The codegen rule divides by element size, yielding a `long` count of elements.

3. **No changes to existing increment operators for non-pointers**: Stage 40 uses 32-bit ops for `int`; we only change behavior for pointer types.
