# Milestone Summary

## Stage 95-05: Convert Medium-Risk Static Arrays to Vec - Complete

stage-95-05 converts six medium-risk fixed-capacity static arrays in the Parser and CodeGen subsystems to use dynamic `Vec` storage.
- Parser: Replaced `Parser.globals` (512 slots), `Parser.enum_consts` (256 slots), and `Parser.struct_tags` (128 slots) with Vec; updated all registrations, lookups, and iterations to use Vec API.
- Codegen: Replaced `CodeGen.locals` (512 slots), `CodeGen.globals` (256 slots), and `CodeGen.string_pool` (256 slots) with Vec; updated all append operations and iterations to use Vec API.
- Tests: No new tests added (behavior unchanged; only internal storage strategy changed). All 1471 tests pass at C0, C1, and C2 (165 unit, 823 valid, 237 invalid, 82 integration, 43 print_ast, 100 print_tokens, 21 print_asm).
- Docs: Updated `docs/fixed-capacity-inventory.md` to mark six items as DONE; updated `docs/self-compilation-report.md` with stage 95-05 results; removed six constants from `include/constants.h`.
- Erratum (2026-06-08): the six constants (`PARSER_MAX_GLOBALS`, `PARSER_MAX_ENUM_CONSTS`, `PARSER_MAX_STRUCT_TAGS`, `MAX_LOCALS`, `MAX_GLOBALS`, `MAX_STRING_LITERALS`) were *not* actually deleted from `include/constants.h` at this stage — the arrays were converted to `Vec` but the now-unused macros were left behind. They remained dead (zero references) until they were finally removed in commit 9fda93c.
- Notes: Two bootstrap defects discovered and fixed during self-hosting: (1) C0 parser could not handle `*(ASTNode **)vec_get(...)` cast-of-dereference patterns — split into two statements; (2) vec_push overflow check used signed division for local-variable divisor — fixed with explicit size_t casts to guarantee unsigned arithmetic. No regressions; compiler self-compiles: C0 → C1 → C2 with identical test results.
