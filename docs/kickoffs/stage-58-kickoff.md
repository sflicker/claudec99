# Stage 58 Kickoff - Controlled Standard-Style Test Headers

## Spec Summary

Add a controlled test include directory at `test/include/` containing small standard-style headers for use by integration tests via angle-bracket includes. This provides a minimal, compiler-controlled environment for testing without depending on platform libc headers.

Four headers will be created:
- `stddef.h`: `size_t` typedef and `NULL` macro
- `stdio.h`: `puts()` and `printf()` declarations  
- `string.h`: `strcmp()` and `strlen()` declarations
- `stdlib.h`: `malloc()` and `free()` declarations

Seven integration test subdirectories will exercise these headers:
- `test_stdio_puts_include`
- `test_stdio_printf_include`
- `test_stddef_size_t_include`
- `test_stddef_null_include`
- `test_string_strcmp_include`
- `test_string_strlen_include`
- `test_stdlib_malloc_free_include`

## Compiler Changes

**None.** This stage is purely test infrastructure—no tokenizer, parser, AST, or codegen changes are required.

## Spec Issues Identified

1. **stdio.h include guard typo** (line 38): `CLUADEC99_STDIO_H` should be `CLAUDEC99_STDIO_H`
2. **Invalid cflags path** (lines 87, 107, 128, 144, 160, 199): `-I/test/include` is not a valid absolute path. Based on integration test conventions, correct path is `-I../../include`
3. **string.h strlen test** (line 170): Uses `printf()` but doesn't include `stdio.h`. Also uses wrong cflags (`-I/usr/include` instead of `-I../../include`)
4. **stdlib.h malloc/free test** (line 196): Missing closing `}` for main function
5. **Missing .status files**: Tests with non-zero exit codes need `.status` files:
   - `test_stddef_size_t_include/.status`: should contain `1`
   - `test_stddef_null_include/.status`: should contain `1`
   - `test_stdlib_malloc_free_include/.status`: should contain `42`

## Implementation Plan

### Phase 1: Create test headers
- Create `test/include/stddef.h` with `size_t` and `NULL`
- Create `test/include/stdio.h` with corrected include guard
- Create `test/include/string.h` with includes
- Create `test/include/stdlib.h` with declarations

### Phase 2: Create test directories
- Create 7 integration test subdirectories under `test/integration/`
- For each test, create: `main.c`, `.cflags`, `.expected` (if output), `.status` (if non-zero exit)
- Use corrected cflags path `-I../../include` for all tests

### Phase 3: Update documentation
- Update `README.md` with new test count (total +7 integration tests)

### Phase 4: Commit
- Stage all new files and commit with proper trailer

## Planned File Changes

**New files:**
- `test/include/stddef.h`
- `test/include/stdio.h`
- `test/include/string.h`
- `test/include/stdlib.h`
- `test/integration/test_stdio_puts_include/main.c`
- `test/integration/test_stdio_puts_include/.cflags`
- `test/integration/test_stdio_puts_include/.expected`
- `test/integration/test_stdio_printf_include/main.c`
- `test/integration/test_stdio_printf_include/.cflags`
- `test/integration/test_stdio_printf_include/.expected`
- `test/integration/test_stddef_size_t_include/main.c`
- `test/integration/test_stddef_size_t_include/.cflags`
- `test/integration/test_stddef_size_t_include/.status`
- `test/integration/test_stddef_null_include/main.c`
- `test/integration/test_stddef_null_include/.cflags`
- `test/integration/test_stddef_null_include/.status`
- `test/integration/test_string_strcmp_include/main.c`
- `test/integration/test_string_strcmp_include/.cflags`
- `test/integration/test_string_strlen_include/main.c`
- `test/integration/test_string_strlen_include/.cflags`
- `test/integration/test_string_strlen_include/.expected`
- `test/integration/test_stdlib_malloc_free_include/main.c`
- `test/integration/test_stdlib_malloc_free_include/.cflags`
- `test/integration/test_stdlib_malloc_free_include/.status`

**Modified files:**
- `README.md` (update test totals)

## Key Decisions

1. **Path correction**: All `.cflags` files will use `-I../../include` based on the established integration test subdirectory structure, not the incorrect `-I/test/include` from the spec.

2. **stdio.h include guard**: Will use `CLAUDEC99_STDIO_H` (corrected spelling) for consistency with other headers.

3. **string.h strlen test**: Will include `<stdio.h>` to support the `printf()` call and use correct cflags.

4. **stdlib.h malloc test**: Will complete the missing closing brace and add `.status` file with exit code `42`.

5. **Exit code files**: All tests expecting non-zero exit codes will have corresponding `.status` files.
