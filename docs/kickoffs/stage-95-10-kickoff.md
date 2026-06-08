# Stage 95-10 Kickoff — Remove Static Char Arrays from parser.h

## Spec Summary

Convert all `char field[MAX_NAME_LEN]` embedded arrays in parser.h structs to `const char *` pointers. Replace strncpy/copy operations with direct pointer assignments. Eliminate intermediate local `char[256]` buffers in parsing functions by using `parser->current.value` directly (persistent lexer-pool storage since stage 95-08).

## Checklist

- [ ] `TypedefEntry.name`
- [ ] `FuncSig.name`
- [ ] `GlobalObjSig.name`
- [ ] `StructTag.tag`
- [ ] `UnionTag.tag`
- [ ] `EnumTag.tag`
- [ ] `EnumConst.name`

## Required Tokenizer Changes

None.

## Required Parser Changes

- `include/parser.h`: 7 field type changes (`char field[MAX_NAME_LEN]` → `const char *field`)
- `src/parser.c`:
  - `parser_register_typedef`: replace strncpy with `entry.name = name`
  - `parser_register_function`: replace strncpy with `sig.name = name`
  - `parser_register_global`: replace strncpy with `new_g.name = name`
  - `parser_register_struct_tag`: replace strncpy with `new_st.tag = tag`
  - `parser_register_union_tag`: simplify two-step push+strncpy to `new_ut.tag = tag` before push
  - `parse_struct_specifier`: change `char tag[256]` to `const char *tag = parser->current.value`
  - `parse_union_specifier`: change `char tag[256]` to `const char *tag = parser->current.value`
  - `parse_enum_specifier`: change `char tag[256]` to `const char *tag = parser->current.value`; change `char ename[256]` to `const char *ename = parser->current.value`; replace EnumTag push+strncpy and EnumConst strncpy with pointer assignments

## Required AST Changes

None.

## Required Code Generation Changes

None.

## Test Plan

Run `./test/run_all_tests.sh` after each struct conversion. Commit after each passing run. After all 7 conversions run `./build.sh --mode=self-host` for full C0→C1→C2 validation.
