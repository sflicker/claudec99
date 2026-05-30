# Milestone Summary

## 75-06: va_arg Integer and Pointer Types - Complete

stage-75-06 ships complete code generation for `va_arg` extraction of GP register class types (int, unsigned int, long, unsigned long, long long, unsigned long long, and pointer types) on x86-64, including both register-save area and overflow stack paths per the SysV AMD64 ABI.

- Tokenizer: No changes.
- Grammar/Parser: No changes.
- AST/Semantics: Validates va_arg type arguments, rejecting small promoted types (char, short, _Bool), aggregates by value, arrays, and void.
- Codegen: Replaced AST_BUILTIN_VA_ARG stub with full SysV AMD64 implementation: gp_offset range check with conditional branch, register-save area path (loads reg_save_area, computes offset, advances gp_offset by 8), overflow stack path (advances overflow_arg_area by 8), and type-appropriate loads (4-byte for int types, 8-byte for long/long long/pointer types).
- Tests: 8 new tests (3 valid, 1 invalid for char, 1 invalid for struct) covering register-save area, overflow stack, pointer dereferencing, string comparison, long long comparison, and mixed-type summation. Full suite 1201/1201 pass (745 valid, 224 invalid, 71 integration, 41 print-AST, 99 print-tokens, 21 print-asm).
- Docs: Updated README.md with stage 75-06 status, revised va_arg feature description, removed va_arg from unsupported features, and updated test totals.
- Notes: Two spec bugs detected and corrected in tests: pick7 function call was incorrectly a comma expression; long long test missing vararg in call and using assignment instead of comparison.
