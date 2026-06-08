# Milestone Summary

## Stage 95-04: Convert Low-Risk Static Usages - Complete

stage-95-04 converts three lowest-risk fixed-capacity static arrays in the parser and code generator to use the `Vec` dynamic growable-array module.
- Grammar/Parser: Replaced `Parser.enum_tags[32]` + `enum_tag_count` with `Vec enum_tags` (EnumTag elements) and `Parser.union_tags[32]` + `union_tag_count` with `Vec union_tags` (UnionTag elements); updated enum tag lookup loop and union tag registration to use Vec API.
- Codegen: Replaced `CodeGen.local_statics[128]` + `local_static_count` with `Vec local_statics` (LocalStaticVar elements); updated local-statics append and emit operations to use Vec API.
- Tokenizer/AST: No changes.
- Tests: No new tests added (behavior unchanged; only internal storage strategy changed). All 1471 tests pass at C0, C1, and C2.
- Docs: Updated `docs/fixed-capacity-inventory.md` to mark three items as DONE (stage 95-04); added self-compilation results; created stage 95-04 kickoff.
- Erratum (2026-06-08): converting the three arrays to `Vec` left their now-unused macros (`PARSER_MAX_ENUM_TAGS`, `PARSER_MAX_UNION_TAGS`, `MAX_LOCAL_STATICS`) defined in `include/constants.h`. They remained dead (zero references) until they were removed in commit 9fda93c.
- Notes: Two latent bootstrap defects found and fixed: (1) `build.sh`'s SRC_FILES was missing `src/strbuf.c` and `src/vec.c` since stage 95-02 (normal cmake build found them via CMakeLists.txt); (2) `vec_reserve` overflow check emitted signed `idiv` for struct-member operands, causing false overflow detection in C1 — rewritten using local `size_t` copies to guarantee unsigned `div`.
