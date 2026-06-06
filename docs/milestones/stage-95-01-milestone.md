# Milestone Summary

## Stage 95-01: Fixed-Capacity Inventory - Complete

Stage 95-01 is a documentation-only stage with no compiler code changes. It produces a comprehensive inventory of all 23 fixed-capacity tables and buffers in the compiler source, classifying each for future dynamic-allocation work.

- Tokenizer: No changes.
- Grammar/Parser: No changes.
- AST/Semantics: No changes.
- Codegen: No changes.
- Inventory: Created `docs/fixed-capacity-inventory.md` cataloging all fixed-capacity items across parser, codegen, preprocessor, type system, and AST subsystems. Each entry records capacity, location, overflow behavior, pointer aliasing, realloc safety, and priority ranking. Key findings: `MAX_BREAK_DEPTH` has no bounds check (silent corruption risk); `PARSER_MAX_STRUCT_FIELDS` has hardcoded overflow check instead of using the constant (silent field loss); `PARSER_MAX_TYPEDEFS` is the tightest commonly-hit limit (64 slots); `PARSER_MAX_FUNCTIONS` is frequently exceeded in large translation units (256 slots). Three subsystems already dynamic: `MacroTable`, `GrowBuf` (preprocessor), `ASTNode.children` (stage 92).
- Tests: None — documentation-only stage with no code changes.
- Docs: `docs/fixed-capacity-inventory.md` created; `docs/kickoffs/stage-95-01-kickoff.md` created.
- Notes: Five spec typos noted but not fixed (cosmetic; intent unambiguous): "capcity"→"capacity", "likes"→"limits", "classfy"→"classify", "savely"→"safely", "artififact"→"artifact". Inventory is a living document; future stages will mark items DONE as they implement dynamic allocation.
