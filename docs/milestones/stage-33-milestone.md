# Milestone Summary

## Stage 33: Struct Assignment - Complete

stage-33 ships whole-struct copy support via assignment and declaration initialization from struct variables of the same type.

- Tokenizer: No changes required.
- Grammar/Parser: No changes required.
- AST/Semantics: No changes required.
- Codegen: Added byte-by-byte copy logic in `src/codegen.c` for `AST_ASSIGNMENT` (struct-to-struct assignment via lvalue member resolution) and struct `AST_DECLARATION` with non-brace initializers. Type compatibility is verified strictly: source and destination must have identical `Type*` pointers. Incompatible struct types are rejected with diagnostic messages.
- Tests: Added 2 valid tests (struct assignment and declaration initialization, both returning 7) and 1 invalid test (incompatible struct type mismatch). Full suite 751/751 pass.
- Docs: Updated `docs/grammar.md` with notes on struct copy initialization. Updated README.md to reflect struct assignment support and revised test totals.
- Notes: Assignment and initialization copy whole struct objects byte-by-byte; no shallow/deep semantics distinction is needed because structs contain only scalars and pointers (in current implementation pointers are not dereferenced on copy).
