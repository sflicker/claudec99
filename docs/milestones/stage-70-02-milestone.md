# Milestone Summary

## Stage 70.02: Line and Column Tracking in Tokens - Complete

stage-70-02 ships source position tracking to every token in the ClaudeC99 compiler.
- Tokenizer: Added `SourceFile` struct (path string) and `line`, `col`, `file` fields to `Token`. Added `lexer_advance()` to update position state when consuming characters. Added `token_set_pos()` to record lexer position into a token.
- Grammar/Parser: No changes.
- AST/Semantics: No changes.
- Codegen: Updated `print_token_row()` to format `[line:col]` position column with dynamic width (zero-padded to match widest line/col values). Updated `print_tokens_mode()` to compute max line/col across all tokens and derive formatting widths.
- Tests: Regenerated all 99 print-tokens expected files to include the new `[line:col]` column. Full suite: 1143/1143 pass (705 valid, 212 invalid, 67 integration, 39 print-AST, 99 print-tokens, 21 print-asm).
- Docs: Updated README through stage 70.02 (line/column tracking in tokens).
- Notes: Location markers (`\x01<line>:<path>\n`) handle include boundaries; lexer owns a SourceFile pool with per-path deduplication; preprocessor injects enter-file and return-to-parent markers at include boundaries.
