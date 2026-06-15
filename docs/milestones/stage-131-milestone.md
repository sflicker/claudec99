# Milestone Summary

## Stage 131: sizeof Produces Unsigned size_t - Complete

stage-131 fixes `sizeof` to produce an unsigned `size_t` result per C99 §6.5.3.4, correcting three expression evaluation bugs where sizeof was incorrectly treated as signed `long`.

- Tokenizer: No changes.
- Grammar/Parser: No changes; sizeof syntax is unchanged.
- AST/Semantics: `node->is_unsigned = 1` set on both `AST_SIZEOF_TYPE` and `AST_SIZEOF_EXPR` nodes in codegen (two-line fix).
- Codegen: When materializing sizeof results, mark the node as unsigned so the existing usual arithmetic conversion infrastructure applies unsigned rules to subsequent operations and comparisons.
- Tests: New regression test `test/valid/expressions/test_sizeof_size_t__6.c` validates all three checks from the spec (sizeof > -1, sizeof - 2 > 0, sizeof < 0). All 1935 tests pass (1252 valid, 259 invalid, 88 integration, 50 print-AST, 100 print-tokens, 21 print-asm; 165 unit).
- Docs: Updated README.md to reflect test counts and correct sizeof type description in feature list. No grammar changes.
- Notes: One bootstrap fix required: added `strtod` declaration to `test/include/stdlib.h` (used by parser FP constant evaluation from stages 126-130). Self-host C0→C1→C2 verified with all 1935 tests passing at every stage.
