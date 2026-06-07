# Stage 95-05 Kickoff

## Summary

Stage 95-05 converts the six MEDIUM-priority fixed-capacity static arrays in the parser and code generator to use the Vec dynamic growable-array module. All six items have Safe Realloc: YES, meaning returned pointers are used locally and never stored in other structures, making the conversion straightforward.

Items to convert:
1. **PARSER_MAX_GLOBALS** → Parser.globals Vec<GlobalObjSig>
2. **PARSER_MAX_STRUCT_TAGS** → Parser.struct_tags Vec<StructTag>
3. **PARSER_MAX_ENUM_CONSTS** → Parser.enum_consts Vec<EnumConst>
4. **MAX_LOCALS** → CodeGen.locals Vec<LocalVar>
5. **MAX_GLOBALS** → CodeGen.globals Vec<GlobalVar>
6. **MAX_STRING_LITERALS** → CodeGen.string_pool Vec<ASTNode*>

---

## Tokenizer Changes

None required.

---

## Parser Changes

### Header Changes (include/parser.h)

Replace three fixed arrays + count fields with Vec fields:

1. **Parser.globals**: change from `GlobalObjSig globals[PARSER_MAX_GLOBALS]` + `int globals_count` to `Vec globals` (element type: GlobalObjSig)
2. **Parser.struct_tags**: change from `StructTag struct_tags[PARSER_MAX_STRUCT_TAGS]` + `int struct_tags_count` to `Vec struct_tags` (element type: StructTag)
3. **Parser.enum_consts**: change from `EnumConst enum_consts[PARSER_MAX_ENUM_CONSTS]` + `int enum_consts_count` to `Vec enum_consts` (element type: EnumConst)

### Implementation Changes (src/parser.c)

1. **parser_init**: Initialize Vec fields with appropriate element sizes
2. **parser_find_global**: Search through Vec; return pointer to element or NULL
3. **parser_register_global**: Push new element to Vec (error on duplicate)
4. **parser_find_struct_tag**: Search through Vec; return pointer to element or NULL
5. **parser_register_struct_tag**: Push new element to Vec (error on duplicate)
6. **parse_struct_specifier**: After parsing struct body, re-find-by-name to avoid dangling pointers when nested type specifiers push to the same Vec
7. **parse_union_specifier**: Same re-find pattern; also fixes latent pointer-aliasing bug
8. **parse_enum_specifier**: Update enum constant registration and lookups to use Vec
9. **Enum constant loops**: Update two read-only loops that iterate over enum_consts

---

## AST Changes

None required.

---

## Code Generation Changes

### Header Changes (include/codegen.h)

Replace three fixed arrays + count fields with Vec fields:

1. **CodeGen.locals**: change from `LocalVar locals[MAX_LOCALS]` + `int local_count` to `Vec locals` (element type: LocalVar)
2. **CodeGen.globals**: change from `GlobalVar globals[MAX_GLOBALS]` + `int global_count` to `Vec globals` (element type: GlobalVar)
3. **CodeGen.string_pool**: change from `ASTNode* string_pool[MAX_STRING_LITERALS]` + `int string_pool_count` to `Vec string_pool` (element type: ASTNode*)

### Implementation Changes (src/codegen.c)

1. **codegen_init**: Initialize Vec fields
2. **codegen_find_var**: Search through Vec; return pointer to element or NULL
3. **codegen_add_var**: Create temp LocalVar, fill it, then vec_push
4. **codegen_find_global**: Search through Vec; return pointer to element or NULL
5. **codegen_add_global**: Use memset-zero'd LocalVar, fill via local pointer, then vec_push at end
6. **codegen_emit_string_pool**: Iterate through Vec to emit pool entries
7. **Scope save/restore**: Directly set Vec.len to restore previous scope boundary (documented Vec behavior)
8. **All references**: Update ~50+ usages of local_count, global_count, and string_pool_count to use Vec.len
9. **Type compatibility**: Use (int)cg->locals.len / (size_t) casts throughout for int/size_t compatibility

---

## Test Plan

1. **Unit tests**: Verify Vec initialization, element insertion, and search operations work correctly for all six conversions
2. **Integration tests**: Run full compiler test suite to ensure no regressions
3. **Scope handling**: Verify that scope save/restore via Vec.len truncation preserves proper variable scoping
4. **Pointer stability**: Confirm that re-find patterns in parse_struct_specifier and parse_union_specifier work correctly
5. **String pool**: Verify that string literal deduplication and emission work correctly with dynamic pool
6. **Edge cases**: Test with programs that approach or exceed previous limits (255+ globals, 255+ struct tags, 2047+ string literals)

---

## Implementation Approach

### Safe Realloc Pattern

All six items have Safe Realloc: YES. Returned pointers from find/register functions are used only locally within the calling function and never stored persistently in other structures.

### Parser-Specific Patterns

- **Re-find-by-name after body parsing**: After parsing a struct/union body, re-find the tag by name to get the updated pointer, avoiding use-after-realloc bugs when nested type specifiers push to the same Vec.

### Code Generator-Specific Patterns

- **Temporary variable pattern (codegen_add_var)**: Create a temp LocalVar on the stack, fill all fields, then vec_push
- **Zero-filled pattern (codegen_add_global)**: memset-zero a GlobalVar, fill fields via a local pointer, then vec_push
- **Scope truncation (scope save/restore)**: Directly set Vec.len to restore scope boundary (no need to iterate or free)

### Version Update

Update `src/version.c` to stage 00950500.

### Inventory Update

Mark all six items as DONE (stage 95-05) in `docs/fixed-capacity-inventory.md`.

---

## Decisions

- Convert **ALL 6 medium-risk items** (not just a subset) to reduce future work and provide consistent API surface
- Fix latent pointer-aliasing bug in **parse_union_specifier** at the same time as parse_struct_specifier fix
- Use **(int)cg->locals.len / (size_t) casts** throughout for int/size_t compatibility
- **Directly set Vec.len** for scope truncation (documented Vec behavior, avoids iteration overhead)
