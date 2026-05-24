# Stage 64 Kickoff: Long Long Type Support

## Spec Summary

Stage 64 adds `long long` type support to the ClaudeC99 compiler, treating it as a signed 64-bit integer alongside its unsigned variant. The stage introduces:

- Two new TypeKind values: `TYPE_LONG_LONG` (signed, 8 bytes) and `TYPE_UNSIGNED_LONG_LONG` (unsigned, 8 bytes)
- Lexer support for `LL`/`ll` and `ULL`/`LLU`/`LLu` suffixes on integer literals
- Parser recognition of `long long` as two consecutive `TOKEN_LONG` tokens with proper validation
- Codegen that treats `long long` identically to `long` (same 64-bit registers and operations)
- Predefined macro: `__SIZEOF_LONG_LONG__ 8`
- Updates to `test/include/limits.h`: add `LLONG_MIN`, `LLONG_MAX`, `ULLONG_MAX`
- Grammar documentation update: rename `<integer_type>` to `<integer_type_specifier>` and document `long long` forms

## Tokenizer Changes Required

1. **Lexer suffix parsing** (`src/lexer.c`):
   - Update integer literal suffix parsing to count L/l characters
   - Handle `LL`/`ll` suffix (double-long) as a single token attribute
   - Support combined suffixes: `LL`, `ULL`, `LLU`, `LLu`, `uLL`, `ULL`, etc.
   - Set `TYPE_LONG_LONG` or `TYPE_UNSIGNED_LONG_LONG` on tokens with appropriate suffixes

## Parser Changes Required

1. **Type specifier parsing** (`src/parser.c`):
   - Modify `parse_type_specifier()` to handle `long long` as two consecutive `TOKEN_LONG` tokens
   - Support forms: plain `long long`, `signed long long`, `unsigned long long`, and `long long int` variants
   - Add validation: reject invalid combinations like `long long long`, `short long`, `long long char`

## AST Changes Required

1. **Type system** (`include/type.h`, `src/type.c`):
   - Add `TYPE_LONG_LONG` and `TYPE_UNSIGNED_LONG_LONG` to the `TypeKind` enum
   - Create singleton instances for both new types
   - Add accessor functions: `type_long_long()`, `type_unsigned_long_long()`
   - Update `type_kind_name()` to return proper names for new types
   - Update `type_is_integer()` to recognize both new types as integers
   - Ensure both types are 8 bytes, same as `TYPE_LONG`

## Code Generation Changes Required

1. **Codegen for long long** (`src/codegen.c`):
   - Add `TYPE_LONG_LONG` and `TYPE_UNSIGNED_LONG_LONG` alongside `TYPE_LONG` in all switch statements and comparisons
   - Update `promote_kind()` to handle promotion of `long long` values
   - Update `uac_is_unsigned()` to recognize `TYPE_UNSIGNED_LONG_LONG` as unsigned
   - Verify sizeof handling returns 8 for both new types
   - Ensure load/store operations use same 64-bit registers as `long`
   - Ensure arithmetic and comparison operations handle 64-bit registers properly
   - Verify function argument/return handling for integer register types

2. **Preprocessor** (`src/preprocessor.c`):
   - Add `__SIZEOF_LONG_LONG__` predefined macro with value `8`

## Test Plan

### Valid Tests
1. `sizeof(long long)` returns 8
2. `sizeof(unsigned long long)` returns 8
3. `long long` variable declaration and assignment with `LL` suffix
4. `unsigned long long` variable with `ULL` suffix
5. Arithmetic operations with `long long` values
6. Comparisons with `long long` values
7. Function parameters and return values of `long long` type
8. Typedef with `long long` and `unsigned long long`
9. `limits.h` defines: verify `LLONG_MIN`, `LLONG_MAX`, `ULLONG_MAX` are defined

### Invalid Tests
1. `long long long` (triple-long) must be rejected
2. `short long` (incompatible modifiers) must be rejected
3. `unsigned signed` (duplicate signedness) must be rejected
4. `long long char` (incompatible type with long long) must be rejected

## Known Issues / Ambiguities

### Typos in Spec (Non-blocking)
- Line 66: "singed" should be "signed"
- Line 73: "lomg" should be "long"
- Line 94: "sizedof" should be "sizeof"
- Line 94: "alighmemtn" should be "alignment"
- Line 168: "TEsts" should be "Tests"

### Test Cases Requiring Fixes
1. Line 164: Test uses `2UUl` (double-U suffix) which is invalid. Should be `2ULL`.
2. Lines 172, 183, 194: Test uses `#ifdef defined(...)` which is invalid. The `#ifdef` directive takes a single identifier only. Should use `#if defined(...)` instead.
3. Test blocks at lines 169-177, 180-188, 195-200 are missing closing `}` in their functions.

All identified issues will be corrected during implementation.

## Implementation Order

1. `include/type.h` and `src/type.c`: Add new TypeKind values and supporting functions
2. `src/lexer.c`: Implement `LL` suffix parsing for integer literals
3. `src/parser.c`: Implement `long long` type specifier parsing with validation
4. `src/codegen.c`: Add `TYPE_LONG_LONG` and `TYPE_UNSIGNED_LONG_LONG` handling throughout
5. `src/preprocessor.c`: Add `__SIZEOF_LONG_LONG__` predefined macro
6. `test/include/limits.h`: Add `LLONG_MIN`, `LLONG_MAX`, `ULLONG_MAX` defines
7. Create test cases in `test/valid/` and `test/invalid/`
8. Update `docs/grammar.md` with grammar changes

## Decisions

- `long long` will be represented internally as two `TOKEN_LONG` tokens during parsing, then normalized to appropriate `TYPE_LONG_LONG` or `TYPE_UNSIGNED_LONG_LONG` singleton instances.
- Codegen for `long long` types mirrors `long` type handling to leverage existing 64-bit infrastructure.
- All spec typos and test issues identified will be corrected silently during implementation without requiring spec updates.
