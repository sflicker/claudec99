# Milestone Summary

## Stage 132: Pointer Equality With Non-Null Constants - Complete

stage-132 relaxes pointer equality (`==` and `!=`) comparisons to accept non-zero integer constant operands, matching the GCC/Clang extension behavior.

- Tokenizer: No changes.
- Grammar/Parser: No changes; comparison syntax is unchanged.
- AST/Semantics: No changes; the validation is purely a semantic relaxation in codegen.
- Codegen: Replaced `is_null_pointer_constant()` check with a new `is_integer_constant()` helper (checks for any `AST_INT_LITERAL`) in the `is_pointer_cmp` validation block. Non-constant integer expressions remain rejected; the error message for such cases changed from "comparing pointer with non zero integer" to "comparing pointer with non-constant integer".
- Tests: Three new tests added: `test_pointer_eq_integer_constants__15.c` (extension behavior), `test_pointer_eq_nonnull_constants__63.c` (strict pointer-to-pointer control test), and `test_ptr_eq_nonnull_int_const__0.c` (moved from invalid to valid). One invalid test removed. All 1937 tests pass (1255 valid, 258 invalid, 88 integration, 50 print-AST, 100 print-tokens, 21 print-asm; 165 unit).
- Docs: Updated README.md test counts and feature description. No grammar changes.
- Notes: The sign-extension widening (movsxd) and 64-bit unsigned comparison path were already correct — only the validation rejection guard needed relaxing. Null pointer constant path (p == 0, p != 0) continues to work. Pointer relational comparisons (<, <=, >, >=) against integers remain rejected as per C99. Self-host C0→C1→C2 verified with all 1937 tests passing at every stage, no source changes needed during bootstrap.
