# Milestone Summary

## Stage 60-01: Static Predefined Target Macros - Complete

Stage 60-01 ships 12 static target/ABI predefined macros reflecting the x86_64 Linux LP64 ABI, injected before preprocessing begins.

- Tokenizer: None.
- Grammar/Parser: None.
- AST/Semantics: None.
- Codegen: None.
- Preprocessor: Added 12 macro definitions to the macro table before user preprocessing: platform macros (`__x86_64__`, `__linux__`, `__unix__`, `__LP64__`, `_LP64`), and size macros (`__CHAR_BIT__`, `__SIZEOF_CHAR__`, `__SIZEOF_SHORT__`, `__SIZEOF_INT__`, `__SIZEOF_LONG__`, `__SIZEOF_POINTER__`, `__SIZEOF_SIZE_T__`). All injected via `macro_define()` calls in `preprocess_file()`, not from a separate header file.
- Tests: 6 new tests added (test_pp_predef_x86_64_ifdef__42.c, test_pp_predef_linux_ifdef__42.c, test_pp_predef_unix_ifdef__42.c, test_pp_predef_lp64_if__42.c, test_pp_predef_sizeof_types_if__42.c, test_pp_predef_char_bit_if__42.c). Full suite 1058/1058 pass.
- Docs: README.md updated with stage reference and feature description.
- Notes: Predefined macros are unconditional (no `#undef`), available in `#if`, `#ifdef`, and normal macro expansion. Stage 59 baseline: 1052 tests. Stage 60-01 adds 6 tests; total 1058.
