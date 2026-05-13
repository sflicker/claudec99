# Milestone Summary

## Stage 40: Unsigned Integer Types and size_t - Complete

stage-40 ships unsigned integer type support: `unsigned char`, `unsigned short`, `unsigned int`, `unsigned long`, and plain `unsigned` (equivalent to `unsigned int`).

- Tokenizer: Added TOKEN_UNSIGNED keyword.
- Grammar/Parser: Extended <integer_type> to accept `[ "unsigned" ] <base_type>` and plain `unsigned`; parse_type_specifier updated; all type-start checks updated to include TOKEN_UNSIGNED; is_unsigned flag propagated through all declaration contexts (block-scope, file-scope, typedefs, parameters).
- AST/Semantics: Added is_unsigned field to ASTNode, LocalVar, and GlobalVar; Usual Arithmetic Conversions (UAC) helper distinguishes signed vs unsigned types in mixed-signedness operations.
- Codegen: emit_load_local/emit_load_global use movzx (zero-extend) for unsigned sizes 1/2 vs movsx (sign-extend) for signed; unsigned division uses div vs idiv; right shift uses shr (logical) vs sar (arithmetic); comparisons use unsigned setb/seta/setbe/setae vs signed setl/setg/setle/setge; unsigned 32→64 widening skips movsxd (x86-64 auto zero-extends).
- Tests: 12 new tests added (10 valid, 2 invalid) covering unsigned loads, division, remainder, right shift, comparisons, plain unsigned, and typedef forms; invalid tests reject `unsigned long int` and `long unsigned`. Full suite 810/810 pass (508 valid, 169 invalid, 24 print-AST, 88 print-tokens, 21 print-asm).
- Docs: grammar.md updated with unsigned type rules; README.md updated to reflect stage 40 with unsigned types and codegen details.
- Notes: Spec explicitly excludes declaration-specifier order permutations (e.g., `long unsigned`); full Usual Arithmetic Conversions edge cases deferred to later stages.
