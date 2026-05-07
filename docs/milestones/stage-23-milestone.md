# Milestone Summary

## Stage 23: Storage Class Basics - Complete

stage-23 ships support for file-scope `extern` and `static` storage class specifiers.

- Tokenizer: Added TOKEN_EXTERN and TOKEN_STATIC keywords.
- Grammar/Parser: Added storage class specifier parsing before type specifier in external declarations; rejects extern with initializer, multiple storage class specifiers, and block-scope storage class; validates linkage consistency (rejects static/extern conflicts, duplicate static definitions, static/non-static function mismatches).
- AST: Added StorageClass enum (SC_NONE, SC_EXTERN, SC_STATIC) and storage_class field to ASTNode; added storage_class field to FuncSig and GlobalObjSig structs.
- Codegen: Skips extern-only declarations in global registration pass; emits `global funcname` NASM directive only for non-static functions.
- Tests: 11 new tests (6 valid, 5 invalid) covering extern objects, extern functions, static objects, static functions, static persistence, conflicting linkage, and block-scope rejection. Full suite 668/668 pass.
- Docs: Updated grammar.md; generated stage-23-kickoff.md.
- Notes: Spec contained minor typos (declaration_spedifiers → declaration_specifiers, implicit int in examples); all tests use explicit int per grammar.
