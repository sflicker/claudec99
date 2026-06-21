# Milestone Summary

## Stage 159: SDL2 Compile Failure: GCC Extension Parsing Fixes - Complete

stage-159 ships six parsing fixes to support GCC extensions and SDL2 header compatibility: array parameters in function-pointer typedefs, `__attribute__`, `__asm__` statements, anonymous struct/union members, `__extension__`, and trailing type qualifiers in parameters.

- **Parser (src/parser.c)**: Three inline parameter-list parsing loops in `parse_declarator` now handle `[...]` array-suffix forms after parameter names (C99 §6.7.5.3p7 array-to-pointer decay); anonymous struct/union members without declarators skip past the semicolon; `__asm__`/`asm` statements are parsed and discarded as no-ops; trailing `const`/`volatile`/`restrict` qualifiers after base types in parameters are consumed and ignored.
- **Preprocessor (src/preprocessor.c)**: New predefined macros `__extension__` (empty expansion), `__attribute__(x)` and `__declspec(x)` (no-op macro replacements in builtin preamble) enable SDL2 headers to proceed further; adjacent string literals in the preamble merged into one (bootstrap fix for C0 string literal concatenation limitation).
- **Tests**: 5 new integration tests cover function-pointer array parameters, `__attribute__` on declarations, `__asm__` statements, anonymous unions in structs, and trailing type qualifiers.
- **Codegen**: No changes.
- **Tokenizer**: No changes.
- **Version**: Bumped to `01590000`.
- **Results**: All 2061 portable tests pass (165 unit, 1286 valid, 261 invalid, 178 integration, 50 print-AST, 100 print-tokens, 21 print-asm); 178 system-include tests pass on Linux x86_64. Self-host C0→C1→C2 verified with one bootstrap fix: merged adjacent string literals in preprocessor preamble.
- **Notes**: SDL2 header parsing now advances further but full compilation requires `SDL_DISABLE_IMMINTRIN_H` since intrinsic headers (xmmintrin.h, bmi2intrin.h) require `unsigned __int128` and vector types (`__m128`) with compound literal initializers — features out of scope for a C99 compiler.
