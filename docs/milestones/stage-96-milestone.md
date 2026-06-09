# Milestone Summary

## Stage 96: Multi-File Compilation and Lifecycle Management - Complete

Stage 96 ships multi-file compilation and complete lifecycle resource management. The compiler now accepts multiple source files per invocation (`ccompiler file1.c file2.c ...`), processes each file with a per-file error-isolation boundary, and frees all acquired resources at the end of each file and the end of compilation.

- **Lifecycle**: Added `reset_error_state()` (util.c) to zero error counters and state between files; `parser_free()` (parser.c) to free all seven owned Vecs; `codegen_free()` (codegen.c) to free eight Vecs plus owned string copies via a new `Vec owned_strings` and `codegen_intern()` helper.
- **Driver / Compiler**: Introduced `compile_one_file()` helper containing the full per-file pipeline; main loop now accumulates source files, processes sysroot once, and calls `compile_one_file()` per file with full resource cleanup (parser, lexer, AST, preprocessed buffer, codegen); overall exit status accumulates results across files.
- **Integration Tests**: Added `run_test.sh` dispatch in `test/integration/run_tests.sh` for test suites that do not use a single `.c` file; added `test_multi_file_success/` (3 files, static local helpers with same name in each, both print 42) and `test_multi_file_error/` (per-file error isolation with `--max-errors=0`).
- **Tests**: All 1483 tests pass (165 unit, 833 valid, 237 invalid, 84 integration, 43 print_ast, 100 print_tokens, 21 print_asm). Self-host C0→C1→C2 cycle completes cleanly.
- **Version**: Bumped VERSION_STAGE to `00960000`.
- **Notes**: C0 bootstrap caveat applied to `codegen_free`: `*(T **)vec_get(...)` patterns split into two statements to avoid C0 parser issues. Per-file error isolation requires `--max-errors=0` (default `--max-errors=1` exits before `longjmp` fires).
