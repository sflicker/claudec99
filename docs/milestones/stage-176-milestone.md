# Milestone Summary

## Stage 176: Fix Semicolon After while/for (Null Statement) - Complete

stage-176 fixes a parser bug where bare `;` (null statements per C99 §6.8.3) were incorrectly rejected in all statement contexts. The expression in `expression_statement` is optional; `parse_statement()` had no case for it, falling through to the expression-statement fallback which unconditionally called `parse_expression()`, causing the error. The fix adds a check before that fallback to detect and consume a bare `;` token, returning an empty `AST_BLOCK`.

- Tokenizer: None.
- Parser: `parse_statement()` in `src/parser.c` adds a null-statement check immediately before the expression-statement fallback. When a bare `;` is found, the token is consumed and an empty `AST_BLOCK` node is returned (same convention used elsewhere for GCC `__asm__` statements). No new AST node types required.
- AST/Semantics: None; empty `AST_BLOCK` nodes (with `child_count == 0`) already compile to zero instructions in codegen.
- Codegen: None; existing codegen for empty blocks already emits zero instructions.
- Tests: Two new portable integration tests demonstrate null statements: `test_while_null_stmt` (verifies `while (0);` and program continuation), `test_for_null_stmt` (verifies a bounded `for` loop with a null body runs to completion). Full suite 2077 portable (165 unit, 1286 valid, 261 invalid, 194 integration [+2], 50 print-AST, 100 print-tokens, 21 print-asm), 194 system-include, 2 optional-library, 5 build-tool — all pass.
- Docs: Version bumped to `01760000` in `src/version.c`. `<expression_statement>` production in `docs/grammar.md` marked optional. New Stage 176 section added to `docs/outlines/checklist.md`. `docs/self-compilation-report.md` updated with stage-176 self-host results (C0/C1/C2 all passed).
- Notes: Empty `AST_BLOCK` convention reused from existing GCC `__asm__` handling — no new codegen patterns. `while(0);`, `for(;;);`, `if(x);`, and bare `;;` all now compile correctly and execute as no-ops.

## Final Results

Full test suite: 2077 portable + 194 system-include + 2 optional-library + 5 build-tool = 2278 total, all pass. Self-host C0→C1→C2 bootstrap completed with zero source changes and all tests passing at every stage.
