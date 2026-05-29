# Milestone Summary

## Stage 75-04: va_start Codegen Foundation - Complete

Stage 75-04 ships code generation for `__builtin_va_start` and `__builtin_va_end` on variadic functions, with register save area initialization and proper ABI compliance.

- Tokenizer: No changes.
- Grammar/Parser: Fixed bug where unnamed array-typedef function parameters (e.g., `va_list` in `int vprintf(const char *, va_list)`) were not receiving the C99 array-to-pointer adjustment. Applied the same adjustment in the early-return path for unnamed parameters in `parse_parameter_declarator()`.
- AST/Semantics: No changes to AST node types or semantic structures; parser fix resolves type chain handling for typedef'd array parameters.
- Codegen: For variadic functions, allocate hidden 304-byte register save area on stack; emit prologue to save rdi/rsi/rdx/rcx/r8/r9 into register save area; `__builtin_va_start` now initializes va_list struct fields (gp_offset, fp_offset, overflow_arg_area, reg_save_area) with values derived from function signature and saved registers; `__builtin_va_end` remains no-op. Added three fields to CodeGen struct: `variadic_reg_save_offset`, `variadic_named_gp_params`, `variadic_named_stack_params`.
- Tests: 1 new integration test (test_va_start_codegen) demonstrating end-to-end va_start initialization and forwarding of va_list to libc vprintf. Full suite 1190/1190 pass.
- Docs: Updated version from "00750300" to "00750400". Added vfprintf, vprintf, vsnprintf declarations to `test/include/stdio.h` with `#include <stdarg.h>`. Updated README.md to reflect stage 75-04, clarify va_start codegen completion, and note that va_arg and va_copy remain stubs.
- Notes: Parser fix required careful handling of unnamed parameters in function prototypes to ensure typedef'd array types receive proper pointer decay. Register save area layout follows System V AMD64 ABI with 48 bytes for 6 GP slots (rdi–r9) and 256 reserved bytes for FP slots (currently unused).
