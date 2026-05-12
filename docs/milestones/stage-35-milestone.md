# Milestone Summary

## Stage 35 – Nested Structs and Arrays of Structs: Complete

Stage 35 ships nested struct members and array-of-struct element access, enabling chained member access (e.g. `r.origin.x`) and indexed member access (e.g. `points[0].x`).

- Tokenizer: No changes.
- Grammar/Parser: No changes.
- AST/Semantics: No changes.
- Codegen: Extended `emit_member_addr()` in `src/codegen.c` with two new cases: `AST_MEMBER_ACCESS` for chained member access on nested struct fields, and `AST_ARRAY_INDEX` for member access on array-of-struct elements.
- Tests: 10 new tests added (7 valid, 3 invalid). Full suite 767/767 pass (481 valid, 153 invalid, 24 print-AST, 88 print-tokens, 21 print-asm).
- Docs: Stage kickoff written.
- Notes: Spec contained three minor typos: sizeof test expected value (said "15", correct answer is 16), alignment test keyword (said "resize" instead of "return"), and nested field name ("helght" vs "height").
