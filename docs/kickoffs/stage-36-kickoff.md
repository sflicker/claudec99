# Stage 36 — typedef alias for complete struct types — Kickoff

## Spec Summary

Add typedef aliases for complete struct types. Two forms are supported:

- **Form 1 (separate definition + typedef)**: `struct Point { int x; int y; }; typedef struct Point Point;`
- **Form 2 (combined definition + typedef)**: `typedef struct Point { int x; int y; } Point;`

Both allow using the alias name as a type specifier in declarations (plain structs and arrays of structs).

## Tokenizer Changes

None required. All relevant tokens (TOKEN_TYPEDEF, TOKEN_STRUCT, TOKEN_IDENTIFIER) already exist.

## Parser Changes

Two minimal fixes in `src/parser.c` — both in the typedef registration path:

- **File-scope** (`parse_external_declaration`, ~line 2230):
  Change `Type *reg_full_type = (typedef_kind == TYPE_POINTER) ? full_type : NULL;`
  to `Type *reg_full_type = (typedef_kind == TYPE_POINTER || typedef_kind == TYPE_STRUCT) ? full_type : NULL;`

- **Block-scope** (`parse_statement`, ~line 1737):
  Same change.

Both fixes ensure the struct's `Type*` is preserved in the `TypedefEntry.full_type` field so that `parse_type_specifier` can return it when the alias name is used as a type.

## AST Changes

None required.

## Code Generation Changes

None required. The existing struct variable codegen already handles `full_type` pointing to the struct `Type*`.

## Test Plan

- `test_typedef_struct_separate__30` — Form 1, return p.x + p.y = 30
- `test_typedef_struct_combined__7` — Form 2, return p.x + p.y = 7
- `test_typedef_struct_array__33` — Form 2 + array of typedef'd structs, sum of all members = 33

## Implementation Order

1. Parser fix (two lines in src/parser.c)
2. Tests (three new test files in test/valid/)
3. Grammar update (docs/grammar.md)
4. Milestone/Transcript artifacts
