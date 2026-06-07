# Milestone Summary

## Stage 95-09: Dynamic String Storage for AST Values - Complete

stage-95-09 replaces the fixed-size `char value[MAX_NAME_LEN]` buffer in `ASTNode` with `const char *value` (a pointer into lexer-owned storage).

- **AST/Semantics**: Changed `ASTNode.value` from fixed 255-byte array to pointer into lexer-managed string storage. `ParsedDeclarator.name` also migrated to pointer. All value assignments now reference lexer storage via `lexer_store_bytes()` rather than copying. Enum constants, case labels, and string literals now bypass buffer overflow risk.
- **Parser**: Updated string literal handling (single and concatenated), char literals, enum constants, and case labels to use lexer storage. Fixed latent use-after-stack bug in case label formatting by storing formatted values in lexer rather than local stack buffers.
- **Codegen**: Added NULL safety check on `node->value[0]` dereference for AST_ASSIGNMENT.
- **Pretty Printer**: Added NULL safety check on `node->value[0]` dereference for AST_ASSIGNMENT.
- **Tests**: 1 new test added (`test_stage_95_09_long_string_literal__42.c`) to verify string literal exceeding old 255-byte limit. Full suite: 1479/1479 pass (165 unit, 828 valid, 237 invalid, 82 integration, 43 print_ast, 100 print_tokens, 21 print_asm). Self-host bootstrapping C0→C1→C2 successful with no failures.
- **Docs**: Fixed capacity inventory updated; README stage description and test counts updated.
- **Notes**: Unnamed function parameters retain `""` (static empty string) rather than NULL to maintain compatibility with `strcmp` and existing callers. Case label bug fix was critical—all 17 switch statement tests were failing before the fix.
