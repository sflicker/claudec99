# Stage 69 Kickoff: Add Memory-Related Standard Function Declarations

## Summary of the Spec

Stage 69 adds five memory-related C standard library function declarations to the controlled stub headers under `test/include/`:

- `void *realloc(void *, size_t)` → `stdlib.h` (note: spec listed `void realloc` but corrected to `void *` per C99 standard)
- `void *memcpy(void *, const void *, size_t)` → `string.h`
- `void *memset(void *, int, size_t)` → `string.h`
- `int memcmp(const void *, const void *, size_t)` → `string.h`
- `char *strchr(const char *, int)` → `string.h`

This stage is purely header/test infrastructure work with no compiler changes required.

## Required Tokenizer Changes

None. The functions being declared use already-supported tokens and syntax.

## Required Parser Changes

None. No new syntactic constructs are introduced.

## Required AST Changes

None. No AST modifications are needed.

## Required Code Generation Changes

None. The stage involves only external function declarations in stub headers, not code generation.

## Test Plan

### Functional Testing

1. **stdlib.h: realloc testing** (`test/integration/test_stdlib_realloc/`)
   - Test calling `realloc()` with valid pointer and size
   - Test reassigning realloc'd memory
   - Verify pointer arithmetic and basic usage patterns

2. **string.h: memory functions testing** (`test/integration/test_string_h_mem_funcs/`)
   - Test `memcpy()` to copy memory blocks
   - Test `memset()` to initialize memory blocks
   - Test `memcmp()` to compare memory blocks
   - Test `strchr()` to find character in string
   - Mix of const/non-const pointer usage as per function signatures

### Header Compatibility

- Verify `test/include/stdlib.h` compiles with realloc declaration
- Verify `test/include/string.h` compiles with new memory function declarations
- Confirm no conflicts with existing declarations (malloc, free, strcmp, strlen)

### Implementation Steps

1. Add `void *realloc(void *, size_t);` to `test/include/stdlib.h`
2. Add four memory functions to `test/include/string.h`:
   - `void *memcpy(void *, const void *, size_t);`
   - `void *memset(void *, int, size_t);`
   - `int memcmp(const void *, const void *, size_t);`
   - `char *strchr(const char *, int);`
3. Create integration tests validating usage patterns
4. Verify all tests pass with the compiler
