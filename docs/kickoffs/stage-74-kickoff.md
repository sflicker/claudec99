# Stage 74 Kickoff - Controlled Header Gap Fill

## Spec Summary

Stage 74 adds four missing standard library header stubs to the controlled headers subset (`test/include/`) to enable testing of common C library functionality:

1. **ctype.h**: Character classification and conversion functions (isalnum, isalpha, isdigit, isspace, isxdigit, islower, isupper, tolower, toupper)
2. **errno.h**: Error handling with `__errno_location()` function and standard error constants (EPERM, ENOENT, ESRCH, EINTR, EIO, ENOMEM, EACCES, EINVAL, ERANGE, ERANGE)
3. **time.h**: Time types and functions (time_t, clock_t typedefs; CLOCKS_PER_SEC macro; time() and clock() functions)
4. **setjmp.h**: Non-local jump facility (jmp_buf typedef as long[32]; setjmp() and longjmp() functions)

**Scope**: Headers only. No compiler source changes. No full host libc compatibility required.

## Required Changes

### Tokenizer Changes
None required.

### Parser Changes
None required.

### AST Changes
None required.

### Code Generation Changes
None required.

## Spec Issues Found and Corrections

1. **time.h line 55**: Typo `typedef log time_t;` should be `typedef long time_t;`
2. **errno.h line 35**: Macro format error `#define errno(*__errno_location())` should be `#define errno (*__errno_location())` (space required to be object-like macro)
3. **errno.h line 44**: Constant typo `EINIVAL` should be `EINVAL` (extra I)
4. **Test 2 (line 102)**: Syntax error `return isalpha('A'); != 0;` should be `return isalpha('A') != 0;` (semicolon in wrong place)

## Planned Changes

### Phase 1: Create Header Files
1. Create `test/include/ctype.h` with character classification and conversion function declarations
2. Create `test/include/errno.h` with error location function and error code constants (applying typo fixes)
3. Create `test/include/time.h` with time types and function declarations (applying typo fix)
4. Create `test/include/setjmp.h` with non-local jump facility declarations

### Phase 2: Create Test Cases
5. Add 8 valid tests to `test/valid/`:
   - `ctype_isdigit.c`: Test isdigit('7')
   - `ctype_isalpha.c`: Test isalpha('A')
   - `ctype_toupper.c`: Test toupper('a')
   - `ctype_isspace.c`: Test isspace('\n')
   - `errno_assignment.c`: Test errno assignment and EINVAL comparison
   - `errno_constant.c`: Test ENOENT constant value
   - `time_basic.c`: Test time() function and time_t type
   - `setjmp_sizeof.c`: Test sizeof(jmp_buf)

### Phase 3: Update Compiler Version
6. Update `src/version.c` VERSION_STAGE to "00740000"

### Phase 4: Update Documentation
7. Update `README.md` to reflect stage 74 completion
8. Update `docs/grammar.md` if needed (no grammar changes expected)

## Implementation Order

1. Create the four header files in sequence, applying spec corrections
2. Create the eight test cases in valid/ directory
3. Verify all tests compile and run with expected results
4. Update version number
5. Update documentation files

## Ambiguities and Decisions

- **errno macro**: Spec defines as function-like `#define errno(*__errno_location())` but correcting to object-like `#define errno (*__errno_location())` allows it to behave like a true lvalue in assignments
- **jmp_buf size**: Defined as fixed 32-element long array; no runtime variation needed
- **time() return value**: Function takes optional pointer argument in full POSIX; stub takes literal 0 for simplicity
- **No actual errno implementation**: The `__errno_location()` is declared but not defined; tests use it syntactically

## Success Criteria

- All four header files created with correct declarations and corrected typos
- All eight test cases compile successfully
- All eight test cases run with expected return values
- Version number updated to 00740000
- Documentation updated
