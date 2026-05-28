# Milestone Summary

## Stage 75-03: Builtin Parsing and Semantic Recognition - Complete

Stage 75-03 ships parsing and semantic recognition of the four GCC `__builtin_va_*` intrinsics produced by `<stdarg.h>` macro expansion.

- Tokenizer: No changes.
- Grammar/Parser: Added recognition of `__builtin_va_start`, `__builtin_va_end`, `__builtin_va_copy`, `__builtin_va_arg` in `parse_primary()` before general identifier resolution. Added `current_func_is_variadic` field to Parser struct to track variadic context. Semantic check: `__builtin_va_start` must appear inside a variadic function and requires exactly 2 arguments; `__builtin_va_arg` parses second argument as type-name.
- AST/Semantics: Added four new ASTNodeType values: `AST_BUILTIN_VA_START`, `AST_BUILTIN_VA_END`, `AST_BUILTIN_VA_COPY`, `AST_BUILTIN_VA_ARG`. For `__builtin_va_arg`, result type stored in node's `decl_type`/`full_type` fields.
- Codegen: No-op implementation; `__builtin_va_start/end/copy` return `void`; `__builtin_va_arg` emits `xor eax, eax` and returns the specified type.
- Tests: 5 new tests (2 print_ast, 3 invalid). Full suite 1189/1189 pass.
- Docs: Updated version from "00750200" to "00750300". Updated `docs/grammar.md` with `<builtin_expression>` production. Updated README.md to reflect stage 75-03, new test counts, and clarification that va_* macros are now parsed/validated.
- Notes: Spec file has title mismatch ("stage 75-02" vs "stage-75-03" filename) and typos (`__buildin_` instead of `__builtin_`, "must ben"); these were corrected during implementation.
