# Milestone Summary

## Stage 70.01: Add Additional Tooling Features - Complete

stage-70-01 ships a versioning system and error management infrastructure for the compiler.
- Tokenizer: No changes.
- Grammar/Parser: Parser error recovery via setjmp/longjmp; replaced ~104 exit(1) pairs with uniform compile_error() calls; added parser_sync() to advance to next declaration boundary.
- AST/Semantics: No changes.
- Codegen: Replaced ~116 exit(1) pairs with compile_error() for uniform error handling; codegen remains terminative (no recovery point).
- Tests: Added 1 integration test (test_max_errors_multi/) demonstrating multi-error collection. Full suite: 1143/1143 pass (705 valid, 212 invalid, 67 integration, 39 print-AST, 99 print-tokens, 21 print-asm).
- Docs: Updated README through stage 70.01; added --version and --max-errors=N to usage section.
- Notes: Version format MM.mm.SSSSSSSS.BBBBB; CMakeLists.txt hooks git rev-list --count for auto-incrementing build number; default max_errors=1 preserves existing behavior (exit on first error); removed dead codegen_warn function.
