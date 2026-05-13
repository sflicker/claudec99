# Stage 40 Kickoff - Unsigned Integer Types and size_t

## Spec Summary

Stage 40 adds support for unsigned integer types to the ClaudeC99 compiler. This includes:

- New unsigned type specifiers: `unsigned`, `unsigned char`, `unsigned short`, `unsigned int`, `unsigned long`
- Plain `unsigned` is equivalent to `unsigned int`
- Proper codegen for unsigned operations: zero-extension, unsigned division/shift, unsigned comparisons
- Basic usual arithmetic conversions for mixed signedness operations

Invalid forms (out of scope): `long unsigned`, `int unsigned`, `unsigned long int`, `signed int`, `signed x`

## Tokenizer Changes

1. Add `TOKEN_UNSIGNED` token in `include/token.h`
2. Update `src/lexer.c` to recognize the `"unsigned"` keyword and emit `TOKEN_UNSIGNED`

## Parser Changes

1. Update `src/parser.c` type-start token checks to include `TOKEN_UNSIGNED`
2. Modify integer type parsing to handle optional `unsigned` prefix
3. Propagate `is_unsigned` field to AST nodes during type parsing
4. Handle invalid cases: reject multiple type specifiers like `long unsigned`, `int unsigned`, `unsigned long int`
5. Update typedef handling for unsigned types

## AST Changes

1. Add `int is_unsigned` field to `ASTNode` in `include/ast.h` to track signedness at the declaration level

## Type System Changes

1. Add unsigned type constructors in `include/type.h`:
   - `type_unsigned_char()`
   - `type_unsigned_short()`
   - `type_unsigned_int()`
   - `type_unsigned_long()`

2. Implement these as singletons in `src/type.c` with `is_signed=0`

## Code Generation Changes

1. Add `int is_unsigned` field to `LocalVar` and `GlobalVar` in `include/codegen.h`
2. Update `src/codegen.c`:
   - Use `movzx` (zero-extend) for loading unsigned char/short
   - Use `div` instead of `idiv` for unsigned division
   - Use `shr` instead of `sar` for unsigned right shift
   - Use `setb`/`seta` (unsigned) instead of `setl`/`setg` (signed) for comparisons
   - Skip `movsxd` sign-extension when widening unsigned int to long

## Test Plan

1. **Signedness in comparisons**: -1 < 1u should return 42 (unsigned comparison)
2. **Unsigned load operations**: char and short loads use `movzx`
3. **Unsigned arithmetic**: division uses `div`, right shift uses `shr`
4. **Typedef support**: `typedef unsigned long size_t` should compile
5. **Invalid cases**: reject `long unsigned`, `int unsigned`, `unsigned long int`

## Implementation Order

1. Tokenizer: Add `TOKEN_UNSIGNED` and lexer support
2. Type system: Add unsigned type constructors
3. Headers: Add fields to AST and codegen structures
4. Parser: Update type parsing and validation
5. Codegen: Implement unsigned-specific instructions
6. Tests: Verify all functionality

## Key Decisions

- Grammar uses `{ "unsigned" }` (zero or more) but implementation interprets as `[ "unsigned" ]` (optional, zero or one)
- Usual arithmetic conversions are simplified: focus on int vs unsigned int; full C99 rules are out of scope
- Store signedness as `is_unsigned` flag rather than separate signed/unsigned types
- Only `unsigned` keyword is supported; `signed` keyword is not
