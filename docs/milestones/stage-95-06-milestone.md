# Milestone Summary

## Stage 95-06: Convert High-Risk Static Usages - Complete

stage-95-06 converts four HIGH-priority fixed-capacity static arrays in the parser and code generator to use the `Vec` dynamic growable-array module.

- **Tokenizer**: No changes.
- **Grammar/Parser**: Two conversions in struct/union body parsing (lines ~394 and ~592 in src/parser.c). `StructField tmp_fields[64]` replaced with local `Vec tmp_fields_vec`; a silent data-loss bug is fixed (overflow check used hardcoded `64` instead of the constant, silently dropping fields beyond 64).
- **AST/Semantics**: No changes.
- **Codegen**: One conversion in CodeGen.break_stack (32-element array replaced with `Vec`). No bounds check existed before any of the 4 write sites (while, do-while, for, switch) — silent buffer overflow risk eliminated. Read sites use `vec_get`; push sites use local `BreakFrame` + `vec_push`; pop sites use `vec_pop`.
- **Overarching changes**: One conversion in `Parser.typedefs` and one in `Parser.funcs` (both replaced with `Vec`); `parser_find_typedef` and `parser_register_typedef` updated to use `vec_get` and `vec_push`; `parser_find_function` and `parser_register_function` updated similarly; unnecessary overflow check removed from function registration. Four constants removed from `include/constants.h`: `PARSER_MAX_STRUCT_FIELDS`, `MAX_BREAK_DEPTH`, `PARSER_MAX_TYPEDEFS`, `PARSER_MAX_FUNCTIONS`.
- **Tests**: No new tests added (behavior unchanged; internal storage only). Full suite 1471/1471 pass (165 unit, 823 valid, 237 invalid, 82 integration, 43 print_ast, 100 print_tokens, 21 print_asm). No regressions.
- **Docs**: README updated to remove four constants from the compiler limits table. Fixed-capacity inventory marked DONE for all four conversions. Self-compilation report updated with stage 95-06 results (C0 → C1 → C2 with no bootstrap issues).
- **Notes**: All four HIGH-priority conversions completed without introducing new bugs. Silent data-loss bug in struct field parsing (fields beyond 64 were dropped) and silent overflow risk in break-depth tracking (no bounds check) are now fixed. No language features were added.
