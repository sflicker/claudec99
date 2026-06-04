# Milestone Summary

## Stage 90: Hexadecimal Integer Literals - Complete

stage-90 ships hexadecimal integer literals with the `0x`/`0X` prefix, allowing integer constants to be expressed in base-16 notation with all standard integer suffixes.

- Tokenizer: Added hex literal detection in the integer literal branch of `lex()`. When `c == '0'` and the next character is `x` or `X`, the lexer consumes the prefix and then reads hex digits using `isxdigit()`. Missing hex digits after the prefix trigger an error; valid hex sequences are parsed via `strtoul(token.value, &end, 16)`. All suffix and type-determination logic is shared with the decimal integer path.
- Grammar/Parser: Updated `<integer_literal>` rule in the grammar to introduce `<decimal_literal>` and `<hex_literal>` sub-productions, with full suffix support (`U`, `L`, `UL`/`LU`, `LL`, `ULL`/`LLU`) on both forms.
- AST/Semantics: No changes; hex literals are treated as plain integer constants, with type determined by value range and suffix.
- Codegen: No changes; hex literals are codegen-neutral (already compile to identical assembly as their decimal equivalents).
- Tests: 7 tests added (5 valid, 2 invalid). Full suite 1300/1300 pass.
- Docs: Grammar updated with hex literal rule; README updated with feature description and test totals.
- Notes: Hex literal parsing follows C99 semantics; `0x` and `0X` are both recognized; missing or invalid hex digits are properly rejected.
