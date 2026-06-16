# Milestone Summary

## Stage 133: Default Argument Promotions In Function Calls - Complete

Stage 133 ships C99 §6.5.2.2p6 default argument promotions for calls to functions declared without a prototype (`int f()`).

- **Tokenizer**: No changes.
- **Grammar/Parser**: Distinguish empty `()` as "no prototype information" (not a zero-parameter prototype); `(void)` remains a zero-parameter prototype. Updated `parser_register_function` to allow no-prototype forward declarations followed by later prototype definitions. Two new AST/FuncSig fields track this state: `ASTNode.is_no_prototype` and `FuncSig.has_no_prototype`.
- **AST/Semantics**: Added `is_no_prototype` flag to `ASTNode` in `include/ast.h` and `has_no_prototype` flag to `FuncSig` in `include/parser.h` to distinguish declarations with no parameter type information from zero-parameter prototypes.
- **Codegen**: Extended float→double promotion in Phase 1 (stack FP args) and Phase 2 (register FP args) of call emission from variadic-only to `is_variadic || is_no_prototype`. Integer narrow-type promotions (char/short → int) are automatically correct via existing `movsx`/`movzx` sign/zero-extend load instructions.
- **Tests**: 2 new tests added; 1939 passed, 0 failed (was 1937 valid tests). New tests: `test_default_argument_promotions__127.c` (7-condition variadic promotion, exits 127) and `test_default_promotions_no_prototype__31.c` (5-condition no-prototype call, exits 31; previously rejected with "parameter count mismatch").
- **Docs**: None (no new syntax).
- **Notes**: Self-host C0→C1→C2 verified; all 1939 tests pass at every stage. The compiler's own source never uses no-prototype declarations, so `is_no_prototype` is always 0 during bootstrap.
