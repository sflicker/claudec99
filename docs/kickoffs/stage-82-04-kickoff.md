# Stage 82-04 Kickoff: Minimal volatile handling

## Summary of the spec

Add minimal support for the `volatile` type qualifier:

- `volatile` is tokenized as a new keyword token `TOKEN_VOLATILE`
- `volatile` parses at all type-qualifier positions (same as `const`)
- `volatile` is preserved in the type via `is_volatile` field on Type and StructField
- No codegen restrictions or enforcement yet (writes to volatile-qualified vars are allowed)

This stage follows the same pattern as stage 82-03 (const in type-name contexts), providing the infrastructure to recognize and track volatile qualifiers throughout the type system.

## Tokenizer changes

Update `/include/token.h` and `/src/lexer.c`:

1. Add `TOKEN_VOLATILE` to the token enum in `token.h` (after `TOKEN_CONST`)
2. Map the "volatile" keyword string in the lexer
3. Add display name for `TOKEN_VOLATILE` in the token display function

## Parser changes

Update `/src/parser.c`:

1. Add `TOKEN_VOLATILE` to all positions where `TOKEN_CONST` currently appears as a type qualifier
2. Update `DeclSpecResult` to include an `is_volatile` field
3. Modify parsing functions that handle qualifiers:
   - `parse_declaration_specifiers()` — recognize and set `is_volatile`
   - `parse_specifier_qualifier_list()` — recognize and set `is_volatile`
   - All pointer declarator parsing to track volatile on pointer types

## AST/Type changes

Update `/include/type.h` and `/src/type.c`:

1. Add `is_volatile` field to `struct Type`
2. Add `is_volatile` field to `struct StructField`
3. Declare `type_volatile_copy()` function to apply volatile to an existing type
4. Implement `type_volatile_copy()` following the pattern of `type_const_copy()`

## Code generation changes

None. The codegen will write to volatile-qualified variables without restriction. Enforcement of volatile semantics (preventing certain optimizations) is deferred to a later stage.

## Test plan

Two new tests in `test/valid/` and `test/print_tokens/`:

1. **Integration test** (`test/valid/test_struct_volatile_int_member__42.c`):
   - Define a struct with a volatile int member
   - Assign and retrieve the value
   - Verify the program compiles and returns 42
   - Validates that volatile qualifiers parse and compile without error

2. **Token test** (`test/print_tokens/test_token_volatile.c` and `.expected`):
   - Verify that "volatile" keyword is tokenized as `TOKEN_VOLATILE`
   - Check token output matches expected display name

## Implementation order

1. Add `TOKEN_VOLATILE` to token definitions and lexer
2. Add `is_volatile` fields to Type and StructField
3. Implement `type_volatile_copy()`
4. Update parser to handle `TOKEN_VOLATILE` at all qualifier positions
5. Run tests to verify parsing and type preservation
6. Update `src/version.c` to stage 00820400
7. Update `docs/grammar.md` with volatile qualifier productions
8. Update `README.md` with feature summary
