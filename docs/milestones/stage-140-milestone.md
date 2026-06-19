# Milestone Summary

## Stage 140: Pointer-Size Typedef Behavior - Complete

stage-140 fixes CC99-013: casts to unsigned typedef types (like `size_t`) did not preserve unsigned arithmetic semantics.

- Tokenizer: No changes.
- Grammar/Parser: Single line added in `parse_cast` (`src/parser.c` ~line 2237) to set `cast->is_unsigned = !cast_type->is_signed`, propagating the target type's signedness flag to the AST node.
- AST/Semantics: `AST_CAST` nodes now correctly record the signedness of their target type, enabling downstream Usual Arithmetic Conversions (UAC) to classify casts to `size_t` and similar unsigned typedef aliases as unsigned operands.
- Codegen: No changes; the `AST_CAST` handler already passes `is_unsigned` through unchanged, so the parser-set value flows correctly to binary operations.
- Tests: 3 new tests (1 integration reproducing the spec's reduced source returning 10, 2 valid). Full suite 1982/1982 pass (1284 valid, 262 invalid, 98 integration, 50 print-AST, 100 print-tokens, 21 print-asm; 165 unit).
- Docs: Milestone and transcript generated; checklist and README updated.
- Notes: **Root cause**: `parse_cast` set `cast->decl_type` from the cast target's `TypeKind` but never set `cast->is_unsigned`, so typedef aliases like `size_t` (resolved to `unsigned long`) lost their signedness on the AST node; UAC then treated both operands of `(size_t)0 - (size_t)1` as signed long, causing `(size_t)0 - (size_t)1 > 0` to evaluate false (signed -1 is not > 0) instead of true. Self-host C0→C1→C2 verified with all tests passing at every stage, no source changes needed during bootstrap.
