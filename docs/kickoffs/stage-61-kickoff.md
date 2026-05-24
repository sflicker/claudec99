# Stage 61 Kickoff: Signed Type and U Literal Suffix Support

## Spec Summary

Add support for the `signed` keyword and formalize the U suffix for integer literals.

**Feature 1: Signed keyword**
- Add `signed` keyword for declaration of signed integer types
- Allowed forms: `signed`, `signed char`, `signed int`, `signed short`, `signed long`, and combinations with trailing `int` qualifier (`short int`, `long int`, `signed short int`, etc.)
- Type normalization: `signed` alone → `int`, `signed int` → `int`, etc. (see spec for full normalization table)
- Reject invalid combinations: `signed unsigned`, `signed void`, `signed struct`, etc.

**Feature 2: U suffix for integer literals**
- Lexer already has partial placeholder implementation (Stage 00-98)
- Formalize support: `U`, `L`, `UL`, `LU` suffixes
- Literal type rules:
  - `123` → `int` or `long` if doesn't fit in `int`
  - `123U` → `unsigned int` or `unsigned long` if doesn't fit in `unsigned int`
  - `123L` → `long`
  - `123UL` → `unsigned long`

## Derived Stage Values

**Affected subsystems:**
- Tokenizer: Add `TOKEN_SIGNED` token
- Lexer: Add `"signed"` keyword mapping; update `token_display_name`
- Parser: Add `signed` handling in `parse_type_specifier`; add optional trailing `int` consumption for `short` and `long` after `signed`; add `TOKEN_SIGNED` to token-check sites
- No AST changes needed (reuse existing unsigned path)
- No codegen changes needed
- No grammar file changes needed (only documentation)

**Type of change:**
- Language feature addition (keyword + type specifier support)

**Scope:**
- Tokenizer and lexer keyword addition
- Parser type specifier parsing
- Test infrastructure

## Planned Changes

### Source Code Changes

**include/token.h:**
- Add `TOKEN_SIGNED` enum value (placed near `TOKEN_UNSIGNED`)

**src/lexer.c:**
- Add `"signed"` keyword in keyword lookup table
- Add entry to `token_display_name` for `TOKEN_SIGNED`

**src/parser.c:**
- In `parse_type_specifier()`:
  - Add new case for `TOKEN_SIGNED` (mirror of `TOKEN_UNSIGNED`)
  - After consuming `signed`, optionally consume `int` for `short` and `long` cases
  - After type determination, check that both `signed` and `unsigned` were not seen; reject with error
  - Handle trailing `int`: e.g., `signed short int` should parse correctly
- Add `TOKEN_SIGNED` to 4 token-check sites:
  - Block-scope declaration validation
  - Cast expression validation
  - Sizeof operator validation
  - File-scope declaration specifiers validation

### Test Plan

**Valid tests (4+):**
1. `signed char` with negative value test (verify signed behavior)
2. `unsigned int` with `U` suffix literal
3. `unsigned long` with `UL` suffix literal
4. `typedef signed char int8_t` with signed char usage

**Invalid tests (4+):**
1. `signed unsigned int x;` (conflicting qualifiers)
2. `unsigned signed int x;` (conflicting qualifiers)
3. `signed void x;` (invalid with non-integer type)
4. `unsigned struct X x;` (invalid with non-integer type)

## Implementation Order

1. Tokenizer: Add `TOKEN_SIGNED` enum
2. Lexer: Add keyword + display name
3. Parser `parse_type_specifier`: Add `signed` case + trailing `int` handling
4. Parser: Add `TOKEN_SIGNED` to 4 token-check sites
5. Create valid test cases
6. Create invalid test cases
7. Verify all tests pass

## Spec Issues Identified

1. **Grammar notation typo** (line 12): Code fence says `bnsf` instead of `ebnf`
2. **Grammar typo** (line 21): `"signed" | unsigned"` — missing opening quote on `unsigned`, should be `"unsigned"`
3. **Test 1** (lines 85-87): Variable declared as `x` but used as `c` in assignment; will implement with consistent `c`
4. **Test 3** (lines 99-103): Missing closing `}` for main function; also line 102 has `return x = 42UL;` which is assignment, not comparison — likely should be `return x == 42UL;`
5. **Test 4** (lines 108-112): Missing closing `}` for main function

## Key Decisions

1. **Trailing int handling**: Optional `int` consumption after `short` and `long` will be handled in `parse_type_specifier` to normalize `signed short int` to `signed short`, etc.

2. **Error checking**: After determining type, check that `signed_seen` and `unsigned_seen` are not both true; reject with clear error message if so.

3. **Test implementation**: Correct the spec issues (variable naming, closing braces, assignment operators) when implementing tests to ensure they are valid and testable.

4. **U suffix**: Already partially implemented; this stage formalizes and ensures it works with all combinations.

## Known Ambiguities

None. Spec is clear on allowed forms, normalization rules, rejection rules, and test cases (modulo the typos listed above).

## Testing Strategy

- Valid tests: Verify signed/unsigned behavior, U suffix parsing, type inference
- Invalid tests: Verify rejection of conflicting qualifiers and invalid type combinations
- Compile and runtime behavior verification
