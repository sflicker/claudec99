# stage-96 Transcript: Multi-File Compilation and Lifecycle Management

## Summary

Stage 96 completes the compiler's lifecycle management and enables multi-file compilation. The compiler now accepts multiple source files per invocation (`ccompiler file1.c file2.c ...`), with each file processed through an independent error-collection boundary and resource cleanup pipeline. A new `reset_error_state()` function zeros per-file error counters between files. Parser, lexer, AST, preprocessor buffers, and code generator resources are now fully freed at the end of each file via new `parser_free()` and `codegen_free()` cleanup functions. A refactored main driver isolates the per-file pipeline in a `compile_one_file()` helper, accumulates source files from the command line, processes the sysroot once, and exits with a status that reflects overall success/failure across all input files. Integration test support is extended to recognize `run_test.sh` scripts for multi-file test suites.

## Changes Made

### Step 1: Error State Management

- Added `reset_error_state()` to `include/util.h` and implemented in `src/util.c`.
- Function zeroes `g_error_count`, `g_error_src_file`, `g_error_src_line`, `g_error_src_col`, and `g_error_jmp_valid`.
- Called between files in the main compilation loop to isolate per-file error state.

### Step 2: Parser Lifecycle

- Added `parser_free(Parser *p)` declaration to `include/parser.h`.
- Implemented in `src/parser.c` to call `vec_free()` on all seven owned Vecs: `funcs`, `globals`, `typedefs`, `enum_consts`, `enum_tags`, `struct_tags`, `union_tags`.
- Element strings remain in lexer-owned storage (not freed by parser).
- Called at end of per-file compilation after AST processing.

### Step 3: Code Generator Lifecycle and String Interning

- Added `Vec owned_strings` to `CodeGen` struct in `include/codegen.h` (placed after `struct_ret_scratch_cursor`).
- Added `codegen_intern(CodeGen *cg, const char *s)` helper function in `src/codegen.c`: calls `util_strdup(s)`, pushes copy into `owned_strings`, returns the copy.
- Updated two `util_strdup` call sites to use `codegen_intern`: one for `LocalStaticVar.label` (~line 3996), one for `GlobalVar.init_label` for string-pointer globals (~line 5081).
- Added `codegen_free(CodeGen *cg)` declaration to `include/codegen.h`.
- Implemented `codegen_free` in `src/codegen.c` to iterate `owned_strings` and free each string (split into two statements: `char **pp = vec_get(...); char *s = *pp;` to accommodate C0 parser limitations), then `vec_free()` all eight Vecs: `owned_strings`, `locals`, `globals`, `break_stack`, `switch_stack`, `user_labels`, `string_pool`, `local_statics`.
- Called at end of per-file code generation.

### Step 4: Driver Refactoring

- Added static `compile_one_file(...)` helper in `src/compiler.c` with the following signature:
  ```c
  static int compile_one_file(
      const char *source_file,
      int print_ast,
      int print_tokens,
      int warnings_are_errors,
      const char **defines,
      int n_defines,
      const char **include_dirs,
      int n_include_dirs
  )
  ```
- This helper encapsulates the full per-file compilation pipeline: lexer creation, preprocessing, optional token/AST printing, parser, optional early exits, code generation, and all resource cleanup (parser_free, lexer_free, ast_free, free(preprocessed), codegen_free).
- Returns 0 on success, nonzero on error (accumulating per-file results).
- Refactored `main()` to:
  - Accumulate source files in a growable `char **source_files` array (grows dynamically).
  - Process sysroot once after all command-line arguments are parsed.
  - Loop over source files, calling `reset_error_state()` + `compile_one_file()` per file.
  - Accumulate overall exit status across files.
  - Free `source_files` array at end.

### Step 5: Integration Test Runner Update

- Modified `test/integration/run_tests.sh` to check for `run_test.sh` in test directories.
- Added dispatch logic: if no `.c` file exists in a test directory, check for `run_test.sh` and execute it, collecting pass/fail results.
- Allows multi-file test suites to define custom build and validation logic.

### Step 6: Multi-File Test Suite — Success Case

- Created `test/integration/test_multi_file_success/` with three source files:
  - `file_a.c`: static function `helper(int x)` returns `x * 2`; public `answer_a()` returns `helper(21)` (42).
  - `file_b.c`: static function `helper(int x)` returns `x + 10`; public `answer_b()` returns `helper(32)` (42).
  - `main.c`: calls both `answer_a()` and `answer_b()`, prints both results (both 42), exits 0.
- `run_test.sh`:
  - Invokes `ccompiler $IFLAGS file_a.c file_b.c` in a working directory with `cd "$WORK" || exit 1`.
  - Verifies both `file_a.asm` and `file_b.asm` exist.
  - Separately compiles `main.c` to `main.asm`.
  - Assembles all three `.asm` files with `nasm -f elf64`.
  - Links all three `.o` files and libc with `gcc -no-pie`.
  - Executes the binary and verifies output is exactly `42\n42\n`.
- Tests static-scope isolation: each file's static `helper` is internal to its file, preventing name collisions.

### Step 7: Multi-File Test Suite — Error Isolation Case

- Created `test/integration/test_multi_file_error/` with two source files:
  - `bad.c`: intentional syntax error (missing semicolon after `int x = 1`).
  - `good.c`: valid C (`int ok(void) { return 99; }`).
- `run_test.sh`:
  - Invokes `ccompiler --max-errors=0 bad.c good.c`.
  - Verifies exit code is nonzero (compilation failed).
  - Verifies `good.asm` exists (proves both files were processed before exiting).
  - Does not verify contents of `bad.asm` (error case, output unreliable).
- Demonstrates per-file error isolation: error in one file does not prevent processing the next file (when `--max-errors=0` is set).

### Step 8: Version Bump

- Bumped `VERSION_STAGE` in `src/version.c` from `"00951200"` to `"00960000"`.

## Final Results

All 1483 tests pass (165 unit, 833 valid, 237 invalid, 84 integration, 43 print_ast, 100 print_tokens, 21 print_asm). No regressions. Self-host bootstrap C0→C1→C2 completes cleanly with all tests passing at each stage.

## Session Flow

1. Read stage 96 spec and supporting documentation.
2. Reviewed lifecycle requirements and multi-file compilation goals.
3. Planned per-file cleanup functions (reset_error_state, parser_free, codegen_free) and driver refactoring.
4. Implemented error-state reset function (util.c).
5. Implemented parser cleanup (parser.c).
6. Implemented code generator cleanup with string interning helper (codegen.c).
7. Refactored main driver with compile_one_file helper and multi-file loop (compiler.c).
8. Added run_test.sh dispatch to integration test runner.
9. Created multi-file success test suite (test_multi_file_success).
10. Created multi-file error-isolation test suite (test_multi_file_error).
11. First test run: test_multi_file_success/run_test.sh failed because make_output_path outputs `.asm` files by basename only (current working directory), not next to source files. Fixed by adding `cd "$WORK" || exit 1` at the start of both run_test.sh scripts to ensure .asm files are created in the test directory.
12. Bumped version to stage 00960000.
13. Ran full test suite: all 1483 tests pass (GCC build).
14. Ran self-host bootstrap C0→C1→C2: all tests pass at each stage.
15. Generated milestone, transcript, and README updates.

## Notes

**C0 Bootstrap Caveat:**

The `codegen_free()` function required splitting double-dereference patterns to avoid C0 parser issues:

```c
// Instead of: char *s = *(char **)vec_get(&cg->owned_strings, i);
// Use:
char **pp = vec_get(&cg->owned_strings, i);
char *s = *pp;
```

This pattern was also applied in stage 95-05 (`vec_push` overflow checks) and ensures compatibility with the C0 bootstrap compiler.

**Error Isolation Requirement:**

Per-file error isolation (processing the next file after an error in the current file) requires `--max-errors=0`. With the default `--max-errors=1`, the compiler calls `exit(1)` before the `longjmp` that would normally restore control to the per-file loop. The test suite demonstrates this behavior explicitly.

**Resource Cleanup Completeness:**

All acquired resources are now freed: parser-owned Vecs (7 total), code-generator-owned Vecs (8 total), code-generator-owned strings (via `owned_strings` Vec and `codegen_intern`), lexer-owned storage, preprocessor output buffer, and AST. No memory leaks accumulate across multiple files in a single invocation.

**Static Scope Isolation:**

The multi-file success test demonstrates that static functions and variables are correctly isolated per-file: both `file_a.c` and `file_b.c` define a static `helper` function with the same name, and they do not collide because each is emitted with a unique label (e.g., `Lstatic_helper_file_a`, `Lstatic_helper_file_b`) in the final assembly.
