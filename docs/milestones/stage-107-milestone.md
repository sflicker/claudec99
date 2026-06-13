# Milestone Summary

## Stage 107: `inline` Keyword, `<assert.h>`, and `va_copy` Codegen - Complete

stage-107 closes three independent C99 gaps: `inline` keyword parsing, assert macro support via a new stub header, and functional `va_copy` codegen (24-byte struct copy on the stack).

- **Tokenizer**: `TOKEN_INLINE` added and recognized in `src/lexer.c`; keyword display name added to `token_type_name()`.
- **Grammar/Parser**: `inline` consumed and discarded in `parse_declaration_specifiers` (parse-and-ignore pattern matching `restrict` and `volatile`). No AST or grammar changes.
- **Codegen**: `AST_BUILTIN_VA_COPY` no-op stub split from `AST_BUILTIN_VA_END`; now emits three 8-byte rax moves to copy the 24-byte SysV AMD64 `va_list` struct. All three words (gp_offset, fp_offset, overflow_arg_area, reg_save_area) copied independently.
- **Headers**: `test/include/assert.h` created with NDEBUG-aware `assert` macro using preprocessor stringification (`#expr`), `__FILE__`, `__LINE__`, `fprintf`, and `abort`. Preprocessor bug fixed: `__FILE__` and `__LINE__` now expand inside function-like macro bodies via static globals `g_expand_source_path`/`g_expand_current_line` in `src/preprocessor.c`.
- **Tests**: 8 new tests added (inline function variants, assert pass/fail/NDEBUG, va_copy, void implicit return). Full suite: 1615 passed, 0 failed. Self-host C0→C1→C2 cycle passes cleanly.
- **Docs**: `docs/self-compilation-report.md` updated with Stage 107 results. Version bumped to `"01070000"` in `src/version.c`.
- **Notes**: None of the three features required new AST nodes. Stage 107 completes inline parsing and assert header support; va_copy is now a functional 24-byte copy rather than a silent no-op. Implicit void function return was already implemented; checklist item ticked retroactively.
