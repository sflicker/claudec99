# stage-95-04 Transcript: Convert Low-Risk Static Usages

## Summary

Stage 95-04 converted three lowest-risk fixed-capacity static arrays in the compiler to use the `Vec` dynamic growable-array module introduced in stage 95-02. All three items had Safe Realloc = YES (no external aliases) and LOW priority in the fixed-capacity inventory.

The selection criteria were: simplest access patterns, smallest blast radius, and minimal usage footprint. No new language features were added. The conversion was entirely internal storage strategy — all external behavior remained unchanged.

During self-hosting verification, two latent bootstrap defects were discovered and fixed: `build.sh`'s hand-maintained SRC_FILES list was missing `src/vec.c` and `src/strbuf.c` (the cmake build found them automatically); and `vec_reserve`'s overflow check emitted signed division for struct-member operands, causing spurious overflow errors in C1.

## Changes Made

### Step 1: Parser enum_tags and union_tags conversion

- Added `#include "vec.h"` to `include/parser.h`.
- Replaced `EnumTag enum_tags[PARSER_MAX_ENUM_TAGS]` and `int enum_tag_count` with `Vec enum_tags` field in the `Parser` struct.
- Replaced `UnionTag union_tags[PARSER_MAX_UNION_TAGS]` and `int union_tag_count` with `Vec union_tags` field in the `Parser` struct.
- Updated `parser_init()` to call `vec_init(&p->enum_tags, sizeof(EnumTag))` and `vec_init(&p->union_tags, sizeof(UnionTag))`.
- Updated `parser_free()` to call `vec_free()` on both new Vec fields.
- Modified enum tag registration loop in the parser to call `vec_push(&p->enum_tags, &tag)` instead of array assignment.
- Modified enum tag lookup loop to iterate using `vec_get(&p->enum_tags, i)` and respect the new Vec length.
- Refactored `parser_find_union_tag()` to use `vec_get()` for lookups.
- Refactored `parser_register_union_tag()` to call `vec_push()` for insertion.

### Step 2: CodeGen local_statics conversion

- Added `#include "vec.h"` to `include/codegen.h`.
- Replaced `LocalStaticVar local_statics[MAX_LOCAL_STATICS]` and `int local_static_count` with `Vec local_statics` field in the `CodeGen` struct.
- Updated `codegen_init()` to call `vec_init(&cg->local_statics, sizeof(LocalStaticVar))`.
- Updated `codegen_free()` to call `vec_free()` on the new Vec field.
- Modified the append operation to call `vec_push(&cg->local_statics, &var)` when registering a local static.
- Updated `codegen_emit_local_statics()` to iterate using `vec_get(&cg->local_statics, i)` and respect the new Vec length.

### Step 3: vec_reserve overflow check fix

- Identified that `vec_reserve()` emitted signed `idiv` when the divisor was accessed via a struct member (`v->elem_size`).
- Rewrote the overflow check to use local `size_t` copies of `v->elem_size` and the subtracted divisor, guaranteeing unsigned `div` emission.
- Before: `if (new_cap > (size_t)-1 / v->elem_size) ...` emitted `cqo; idiv` (signed quotient).
- After: `size_t divisor = v->elem_size; if (new_cap > (size_t)-1 / divisor) ...` emits `div` (unsigned quotient).

### Step 4: Bootstrap infrastructure fixes

- Updated `build.sh` SRC_FILES to explicitly include `src/strbuf.c` and `src/vec.c` (these modules were present since stage 95-02 but the hand-maintained build script was not updated; the cmake build found them automatically).
- These fixes were necessary to enable successful C0 → C1 → C2 self-hosting.

### Step 5: Inventory and version updates

- Updated `docs/fixed-capacity-inventory.md` to mark three items as DONE (stage 95-04):
  - `PARSER_MAX_ENUM_TAGS` (32 slots) converted.
  - `PARSER_MAX_UNION_TAGS` (32 slots) converted.
  - `MAX_LOCAL_STATICS` (128 slots) converted.
- Updated `src/version.c` VERSION_STAGE to "00950400".
- Updated `docs/self-compilation-report.md` with stage 95-04 results.

## Final Results

All 1471 tests pass at C0, C1, and C2:
- 165 unit
- 823 valid
- 237 invalid
- 82 integration
- 43 print_ast
- 100 print_tokens
- 21 print_asm

No regressions. The compiler successfully self-compiles: C0 (GCC-built) → C1 (bootstrap) → C2 (self-hosted) with identical test results at each stage.

## Session Flow

1. Read spec and fixed-capacity-inventory to identify lowest-risk conversion targets.
2. Reviewed Parser and CodeGen structs to understand current fixed-array usage patterns.
3. Analyzed each target array for external aliases and access patterns (all three had Safe Realloc = YES).
4. Implemented parser enum_tags and union_tags Vec conversion.
5. Implemented codegen local_statics Vec conversion.
6. Built with cmake and ran full test suite (all tests passed at C0).
7. Ran bootstrap self-hosting build (build.sh --mode=self-host).
8. Discovered and debugged vec_reserve overflow check emitting signed idiv for struct-member divisor.
9. Fixed overflow check by using local size_t copies to guarantee unsigned div.
10. Discovered build.sh SRC_FILES missing vec.c and strbuf.c; updated bootstrap script.
11. Re-ran self-hosting cycle: C0 → C1 → C2 with all tests passing at each stage.
12. Updated inventory, version, and documentation files.
13. Recorded final results.
