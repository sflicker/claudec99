# Milestone Summary

## Stage 61: Signed type and U suffix support - Complete

stage-61 ships the `signed` keyword and U/L suffix formalization for integer literals and type combinations.

- Tokenizer: Added TOKEN_SIGNED keyword; U/L suffixes already supported.
- Grammar/Parser: Added sign_specifier rule and TOKEN_SIGNED case in parse_type_specifier(), handling `signed` alone (→ int), `signed char`, `signed short`, `signed int`, `signed long` with optional trailing `int` for short and long forms; rejects `signed unsigned` or `unsigned signed` combinations; added optional trailing `int` for `short int` and `long int`; updated 4 type-check sites to recognize TOKEN_SIGNED.
- AST/Semantics: No changes (type representation unchanged).
- Codegen: No changes (code generation unchanged).
- Tests: 14 tests added (10 valid, 4 invalid); 1 test migrated from invalid to valid (unsigned long int now valid). Full suite 1074/1074 pass.
- Docs: Updated docs/grammar.md with sign_specifier and integer_suffix rules. Kickoff and spec reviewed.
- Notes: Spec contained minor typos in grammar notation and test cases; all corrected during implementation.
