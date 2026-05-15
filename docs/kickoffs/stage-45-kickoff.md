# stage-45 Kickoff

## Summary of the spec

Stage 45 introduces integration testing with standard library functions. The key goals are:
- Allow useful libc-style function prototypes (void * returns, const char * parameters)
- Support external function declarations with system libc linking
- Create a new test/integration/ directory with a test harness supporting companion files
- Migrate 7 existing valid tests with .expected files to the new integration suite
- Add 4 new integration tests using puts, strcmp, strlen, and malloc/free

## Required tokenizer changes

None.

## Required parser changes

None.

## Required AST changes

None.

## Required code generation changes

At the function-call argument-passing site in src/codegen.c, replace the strict `pointer_types_equal` check with `pointer_types_assignable` to allow void* parameters to accept object pointers and vice versa, matching the existing rule used at initializers and returns.

## Test plan

**Test harness:**
- Create test/integration/run_test.sh to build and run individual integration tests
- Create test/integration/run_tests.sh to run all integration tests
- Build sequence: ccompiler <src>; nasm -f elf64; cc -no-pie obj -o bin <libs>
- Companion files: BASENAME.expected (output), BASENAME.libs (extra -l flags), BASENAME.args (CLI args), BASENAME.input (stdin), BASENAME.status (exit code if non-zero)

**Migrate 7 existing valid tests** (drop __<exitcode> suffix, write .status only if non-zero):
- test_file_scope_char_array_string_init
- test_file_scope_char_ptr_string_init
- test_file_scope_ptr_array_string_init
- test_file_scope_string_field_table (status=2)
- test_libc_puts_hello
- test_libc_puts_two_calls
- test_puts

**Add 4 new integration tests:**
- test_libc_puts_size_t_main_42 (puts with size_t, return 42)
- test_libc_strcmp_different (strcmp with puts, output "DIFFERENT")
- test_libc_strlen (strlen, output "CORRECT")
- test_libc_malloc_free (malloc/free with void* handling)

**Top-level test updates:**
- Update test/run_all_tests.sh to include `integration` in the suite list
- Update aggregate test totals in README.md

**Documentation updates:**
- Update README.md: bump "Through stage..." line and test totals; mention integration suite and libc prototype support
- No grammar changes to docs/grammar.md

## Ambiguities and spec issues

1. Companion-file table names ".libs" but shell snippet says ".lflags" (with `LFAGS_FILE` typo). Going with ".libs".
2. Companion-file table names ".status" but prose later calls it ".exticode" (typo). Going with ".status".
3. strcmp sample is missing a `}` before `else {` (line 98). Will fix when transcribing.
4. strlen sample has unclosed string literal `"INCORRECT)` (line 118). Will fix when transcribing.
5. The compiler pointer-type compatibility gap (strict pointer_types_equal vs assignable) is uncovered by malloc/free; not explicitly called out in spec but required for compilation.

## Implementation order

1. Tokenizer (no changes)
2. Parser (no changes)
3. AST (no changes)
4. Codegen (pointer-assignability fix)
5. Tests (harness + migrated tests + new tests)
6. Docs (README.md update)
7. Commit
