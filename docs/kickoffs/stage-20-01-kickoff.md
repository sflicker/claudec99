# Stage 20-01 Kickoff: Declarator Refactor

## Spec Summary

This stage refactors the parser's grammar to separate type specifiers from declarators, moving pointer stars, array suffixes, and identifiers into a proper declarator context. This is a pure parser refactor with no AST or codegen changes—all 376 existing tests must continue to pass.

**Before:**
```
int *p;
  -> <type> = int *
  -> identifier = p
```

**After:**
```
int *p;
  -> <type_specifier> = int
  -> <declarator> = *p
```

The compiler still builds proper semantic types internally; only the syntax parsing changes.

## Derived Values

- **Refactor scope:** parser.c only (no tokenizer, AST, or codegen changes)
- **New grammar concepts:** `<type_specifier>`, `<type_name>`, `<declarator>`, `<direct_declarator>`, `<function_declarator>`, `<parameter_declarator>`
- **New parser helper struct:** `ParsedDeclarator` (internal use during parsing)
- **New parser functions:** `parse_type_specifier`, `parse_type_name`, `parse_declarator`
- **Functions to refactor:** 5 existing parser functions that currently handle types and identifiers
- **Test impact:** All 376 valid tests must pass; no behavior changes expected

## Tokenizer Changes

None. The tokenizer is unchanged.

## Parser Changes

**New internal helper struct in parser.c:**
```c
typedef struct {
  // Stores parsed declarator components:
  // - pointer_count (number of '*' tokens)
  // - identifier token
  // - array suffix (size if present)
} ParsedDeclarator;
```

**New parser helper functions:**
1. `parse_type_specifier()` – parses a single keyword: `char`, `short`, `int`, or `long`
2. `parse_type_name()` – parses type_specifier + optional `*` stars (used in casts, sizeof)
3. `parse_declarator()` – parses `*` stars + direct_declarator (identifier or identifier with array suffix)

**Functions to refactor:**
1. `parse_declaration()` – change from `<type> <identifier> [array] [init] ";"` to `<type_specifier> <declarator> [init] ";"`
2. `parse_function()` – change from `<type> <identifier> (params)` to `<type_specifier> <function_declarator> (params)`
3. `parse_parameter_declaration()` – change from `<type> <identifier>` to `<type_specifier> <parameter_declarator>`
4. `parse_cast_expression()` – change `"(" <type> ")"` to `"(" <type_name> ")"`
5. `parse_sizeof_expression()` – change `"sizeof" "(" <type> ")"` to `"sizeof" "(" <type_name> ")"`

## AST Changes

None. Existing AST nodes remain unchanged. The parser may use the internal `ParsedDeclarator` helper during parsing, but final AST output (especially type and identifier storage) continues as before.

## Code Generation Changes

None expected. Codegen operates on final semantic types attached to AST nodes. If any adjustments are needed, they will be minimal adaptations to the minor parser refactor.

## Test Plan

1. **Baseline:** Run all 376 valid tests; confirm they all pass before making changes
2. **Incremental testing:** After each parser function refactor, run the full test suite
3. **Grammar verification:** After updating docs/grammar.md, verify it matches the refactored parser behavior
4. **Expected result:** All 376 valid tests pass with no functional changes

## Implementation Order

1. **Parser refactor** – add ParsedDeclarator struct, implement the three new helper functions
2. **Parser refactor** – update the five existing functions to use the new helpers
3. **Grammar update** – update docs/grammar.md to reflect the new grammar rules
4. **Testing** – confirm all 376 tests pass

## Known Ambiguities

None identified. The refactor is primarily syntactic restructuring to separate concerns (type specifiers vs. declarators).

## Decisions

- **Single declarator per declaration:** For this stage, only one declarator per declaration is supported. Comma-separated declarations are out of scope.
- **ParsedDeclarator scope:** Internal helper only; not exposed to AST or codegen.
- **Array and pointer handling:** Both remain in the declarator; semantic type construction happens in the parser after parsing the declarator.
