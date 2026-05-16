# Stage 47 Kickoff: Multi-File Integration Test Support

## Spec Summary

Stage 47 restructures the integration test harness to support multi-file tests. Each test will move from a flat file layout (test/integration/test_name.c) into its own subdirectory (test/integration/test_name/test_name.c). The test harness will compile every .c file in a test subdirectory to a separate object file, then link all objects together with cc -no-pie. All 12 existing tests are migrated to the new structure, and a new two-file test (test_multi_file_add) is added to demonstrate multi-file compilation.

## Compiler Impact

None. This is pure test infrastructure work. No changes to tokenizer, parser, AST, or code generation.

## Required Changes

### Test Infrastructure
1. **test/integration/run_tests.sh** — Rewrite to:
   - Iterate over subdirectories in test/integration/
   - For each subdirectory, compile all .c files to separate object files
   - Link all objects from that test directory together with cc -no-pie
   - Apply existing companion file logic (.expected, .args, .input, .status, .libs)

2. **test/integration/run_test.sh** — Update to:
   - Accept a source file and compile all .c files in its directory
   - Assemble and link all resulting objects
   - Run and verify exit code and output

3. **Migrate existing 12 tests** — Move from flat to directory structure:
   - test_libc_puts_hello
   - test_libc_puts_two_calls
   - test_file_scope_char_array_string_init
   - test_file_scope_char_ptr_string_init
   - test_file_scope_ptr_array_string_init
   - test_file_scope_string_field_table
   - test_puts
   - test_libc_puts_size_t_main_42
   - test_libc_strcmp_different
   - test_libc_strlen
   - test_libc_malloc_free
   - test_argv_puts

4. **Add new multi-file test (test_multi_file_add)** — Demonstrates compilation of multiple source files:
   - test/integration/test_multi_file_add/test_multi_file_add.c — main calls add(3, 5), expects exit status 8
   - test/integration/test_multi_file_add/add.c — add function implementation
   - test/integration/test_multi_file_add/test_multi_file_add.status — contains 8

## Implementation Order

1. Rewrite run_tests.sh to use subdirectory layout
2. Update run_test.sh to compile all .c files in a directory
3. Migrate all 12 existing tests into subdirectories
4. Create test_multi_file_add with two-file structure
5. Verify all tests pass with new harness

## Spec Ambiguities and Decisions

- **Typo: "support flies"** (line 13) → Interpreted as "support files"
- **Typo: "exit statue"** (line 17) → Interpreted as "exit status"
- **Typo: "it's own"** (line 8) → Should be "its own"
- **Unnamed multi-file test** — Spec shows code example without explicit test name; using test_multi_file_add as the test directory and main file name
- **Companion file locations** — Placing .status, .expected, .args, .input, .libs in test subdirectories alongside the .c files

## Test Plan

1. Run run_tests.sh and verify all 12 migrated tests pass
2. Run test_multi_file_add and verify it compiles, links, runs, and returns exit status 8
3. Verify new multi-file test passes with run_tests.sh
4. Run full integration test suite to confirm no regressions

## Notes

- Companion file logic (.expected, .libs, .args, .input, .status) remains unchanged; only their location moves to match test subdirectories
- All .c files in a test subdirectory are compiled and linked together; tests are no longer restricted to a single source file
- Link command uses cc -no-pie to enable standard C library linking with crt0
