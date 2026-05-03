# Milestone Summary

## Stage 17-03: sizeof Arrays - Complete

stage-17-03 ships compile-time `sizeof()` support for declared array variables, returning the total byte size of the array.

- Tokenizer: No changes required.
- Grammar/Parser: No changes required.
- AST/Semantics: No changes required.
- Codegen: Modified `AST_SIZEOF_EXPR` handler in `src/codegen.c` to detect array variables early and emit the pre-computed total byte size from `full_type->size`, bypassing type decay.
- Tests: 6 new tests added (5 valid, 1 invalid). Full suite 583/583 pass.
- Docs: README.md updated with stage reference and test counts; kickoff artifact already generated.
- Notes: `sizeof(int[10])` (type name form) correctly rejected at parse time as out of scope; implementation preserves array type by intercepting before `sizeof_type_of_expr()` to avoid pointer decay.
