# Milestone Summary

## Stage 112: FP Calling Convention, va_arg for Double, and math.h - Complete

Stage 112 ships complete floating-point calling convention support via the SysV AMD64 ABI, enabling FP function arguments and return values, variadic calls with FP extra arguments, `va_arg` for `double`, and a `test/include/math.h` stub header.

- Tokenizer: No changes.
- Grammar/Parser: No changes.
- AST/Semantics: No changes.
- Codegen: FP argument classification and passing in xmm0–xmm7 (independent from GP registers); non-variadic prologues move xmmN to local stack slots; variadic prologues save xmm0–xmm7 to the 176-byte register-save area (48 GP + 128 XMM); `al` set to XMM count before variadic calls; `va_arg(ap, double)` reads fp_offset and loads from register-save area or overflow-arg-area; three bootstrap bugs fixed (movaps alignment at xmm0=[rbp-128], compute_call_layout decl_type=0 for variadic extras, [MAX_CALL_LAYOUT_ITEMS + 2] arithmetic in local array size).
- Tests: 7 new FP call/variadic tests; 1650/1650 pass (965 valid, 255 invalid, 86 integration, 50 print-AST, 100 print-tokens, 21 print-asm; 165 unit).
- Docs: `test/include/math.h` stub added; README updated; floating-point plan marked complete.
- Notes: `va_arg(ap, float)` rejected per C99 §6.5.2.2p6 (float promoted to double in variadic calls). `test/valid/run_tests.sh` extended with `.libs` companion-file support for extra link flags (e.g., `-lm`).
