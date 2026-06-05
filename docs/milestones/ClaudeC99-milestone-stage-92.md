# Milestone Summary

## stage 92: Self-Compilation Validation - Complete

stage-92 achieves full self-compilation of the ClaudeC99 compiler. The compiler (C0, built with gcc) successfully compiles itself to produce C1, which passes all 1306 tests. C1 then compiles itself to produce C2, which also passes all 1306 tests, confirming bootstrapping stability.

- **Tokenizer/Parser**: No changes (leveraged stage 91-01 struct-by-value support).
- **AST/Semantics**: No new features; validated existing AST correctness through self-compilation.
- **Codegen**: Fixed six silent bugs discovered during self-compilation:
  - Struct-by-value array-element assignment (`arr[i] = struct_func()`)
  - Struct-by-value member-dot assignment (`obj.member = struct_func()`)
  - Struct-by-value member-arrow assignment (`ptr->member = struct_func()`)
  - Struct-by-value deref assignment (`*ptr = struct_func()`)
  - sizeof of arrow-access array/struct/union members
  - sizeof of subscripted-struct members
- **Constants/Infrastructure**: Added `MAX_CALL_LAYOUT_ITEMS` to `constants.h`; added `is_static_linkage` field to `GlobalVar`; emitted `global` NASM directives for non-static file-scope variables.
- **Tests**: All 1306 tests pass (823 valid, 237 invalid, 82 integration, 43 print-AST, 100 print-tokens, 21 print-asm). No regressions.
- **Docs**: Updated stub headers (`time.h`); updated two `print_asm` expected files for new `global` directives; bumped version to 01.02 with stage 92 marker.
- **Notes**: Self-compilation is now achieved and reproducible. C0, C1, and C2 each compile successfully with identical test results, confirming compiler stability.
