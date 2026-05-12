# Milestone Summary

## Stage 36 — typedef alias for complete struct types - Complete

stage-36 ships typedef aliasing for complete struct types, supporting both separate definition + typedef and combined typedef struct definition forms.

- Tokenizer: no changes required.
- Grammar/Parser: two minimal one-line fixes in typedef registration (both block-scope and file-scope paths) to store the struct's `Type*` in `TypedefEntry.full_type` when the outermost kind is `TYPE_STRUCT`. No lookup-side changes needed because the existing `entry->full_type` check already handled it correctly once full_type was populated.
- AST/Semantics: no changes required.
- Codegen: no changes required; existing struct variable codegen already uses `full_type` correctly.
- Tests: 3 new tests added (separate definition, combined definition, array of typedefs). Full suite 770 passed / 770 total (up from 767).
- Docs: grammar.md Typedefs restriction comment updated; stage-36-kickoff.md written.
- Notes: The fix was purely in typedef registration, not lookup. The architecture correctly separated registration (storing full_type) from lookup (retrieving full_type).
