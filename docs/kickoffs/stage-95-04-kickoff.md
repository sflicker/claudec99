# Stage 95-04 Kickoff: Convert Low-Risk Static Usages

## Summary

Stage 95-04 converts the three lowest-risk fixed-capacity static arrays in the
compiler to use the `Vec` dynamic growable-array module introduced in stage 95-02.
No new language features are added.

Items converted in this stage (all LOW priority, Safe Realloc = YES):

| Item | Old | New |
|------|-----|-----|
| `Parser.enum_tags` | `EnumTag[32]` + `int enum_tag_count` | `Vec` (EnumTag) |
| `Parser.union_tags` | `UnionTag[32]` + `int union_tag_count` | `Vec` (UnionTag) |
| `CodeGen.local_statics` | `LocalStaticVar[128]` + `int local_static_count` | `Vec` (LocalStaticVar) |

Selection rationale — these three share:
- **Safe Realloc = YES**: no external pointer aliases; returned pointers are
  only used locally within the calling function before any further push.
- **LOW priority** in the inventory: lowest urgency, simplest access patterns
  (linear scan + append-and-fill).
- Smallest blast radius: each is used in only one source file.

Items NOT converted in this stage (deferred):
- HIGH priority (MAX_BREAK_DEPTH, PARSER_MAX_STRUCT_FIELDS, PARSER_MAX_TYPEDEFS,
  PARSER_MAX_FUNCTIONS) — more complex; reserved for follow-on stages.
- MEDIUM priority (PARSER_MAX_GLOBALS, MAX_LOCALS, MAX_GLOBALS, etc.) — deferred.
- Safe Realloc = NO (FUNC_TYPE_MAX_PARAMS, FUNC_MAX_PARAMS, MAX_SWITCH_LABELS,
  MAX_USER_LABELS) — require deeper struct refactoring.

## Required Changes

### Tokenizer
None.

### Parser (`include/parser.h`, `src/parser.c`)
- Add `#include "vec.h"` to `parser.h`.
- Replace `EnumTag enum_tags[PARSER_MAX_ENUM_TAGS]` + `int enum_tag_count`
  with `Vec enum_tags` in the `Parser` struct.
- Replace `UnionTag union_tags[PARSER_MAX_UNION_TAGS]` + `int union_tag_count`
  with `Vec union_tags` in the `Parser` struct.
- In `parser_init`: call `vec_init` for both new Vec fields.
- Update enum_tags loops to use `parser->enum_tags.len` and `vec_get`.
- Update `parser_find_union_tag` and `parser_register_union_tag` to use Vec API.

### AST
None.

### Code Generator (`include/codegen.h`, `src/codegen.c`)
- Add `#include "vec.h"` to `codegen.h`.
- Replace `LocalStaticVar local_statics[MAX_LOCAL_STATICS]` + `int local_static_count`
  with `Vec local_statics` in the `CodeGen` struct.
- In `codegen_init`: call `vec_init(&cg->local_statics, sizeof(LocalStaticVar))`.
- Remove overflow guard (Vec grows automatically).
- Update append pattern to `vec_push` + `vec_get`.
- Update `codegen_emit_local_statics` loops to use `Vec` API.

### Version
`VERSION_STAGE` → `"00950400"`.

### Inventory
Mark the three converted items as ✓ DONE (stage 95-04) in
`docs/fixed-capacity-inventory.md`.

## Test Plan

- All 1471 existing tests must pass unchanged (165 unit, 823 valid, 237 invalid,
  82 integration, 43 print_ast, 100 print_tokens, 21 print_asm).
- No new tests are needed: behavior is unchanged; only the internal storage
  strategy changes.
- Self-host cycle (C0→C1→C2) validates the converted Vec operations work under
  bootstrap with the compiler's own code.
