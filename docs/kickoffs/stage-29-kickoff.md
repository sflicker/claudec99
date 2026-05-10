# Stage 29 Kickoff: Enum Support

## Summary of the Spec

Stage 29 adds C-style enum support as integer constants. Enums map all enumerator names to compile-time integer values, making them available as identifiers that fold to `AST_INT_LITERAL` nodes during codegen.

Supported features:
- Anonymous enums: `enum { A, B, C };`
- Named enum tags: `enum Color { RED, GREEN, BLUE };` with `enum Color c = BLUE;`
- Explicit integer literal values: `OK = 0, BAD = 10`
- Explicit character literal values: `A = 'A'`
- Auto-increment from last explicit value (default or after explicit assignment)
- Trailing comma in enumerator list: `enum { A, B, C, };`

Invalid cases rejected:
- Duplicate enumerator names in the same scope
- Undefined enum tag reference: `enum Missing x;`
- Non-literal explicit values (expressions like `1 + 2`)

Out of scope: typedef enum, forward enum declarations, fixed underlying types, full constant expressions.

## Required Tokenizer Changes

Add `enum` as a reserved keyword:
- Add `TOKEN_ENUM` token type
- Lex `"enum"` as `TOKEN_ENUM`
- Assign display name `"'enum'"`

## Required Parser Changes

### New Data Structures
In `src/parser.c` (or header):
- `EnumConst` struct: `{ char name[256]; long value; }`
- `EnumTag` struct: `{ char tag[256]; int is_defined; }`

### New Parser Fields
Add to the `Parser` structure:
- `EnumConst enum_consts[PARSER_MAX_ENUM_CONSTS]` (limit: 256)
- `int enum_const_count`
- `EnumTag enum_tags[PARSER_MAX_ENUM_TAGS]` (limit: 32)
- `int enum_tag_count`

### Core Parsing Functions

**`parse_enum_specifier()`**: New function consuming enum syntax
- Parses `enum [tag] [ { list } ]` productions from the grammar
- Handles anonymous enums (no tag) and named enums (with tag)
- Parses enumerator list with auto-increment logic
- Validates no duplicate enumerator names
- Validates referenced tags are defined
- Returns `type_int()` for all enums

**`parse_type_specifier()`**: Add enum support
- Add case for `TOKEN_ENUM`
- Call `parse_enum_specifier()` to handle enum, return `type_int()`

**`parse_declaration_specifiers()`**: Add token recognition
- Add `TOKEN_ENUM` as a recognized type specifier start token

**`parse_statement()` and `parse_external_declaration()`**: Add declaration guards
- Block-scope: add `TOKEN_ENUM` to standalone-declaration detection (reject `enum { A, B };` as a statement)
- File-scope: add `TOKEN_ENUM` to standalone-declaration detection (reject `enum { A, B };` at file level)
- Both must require a declarator or declarator list to follow the type specifier

### Primary Expression Integration
In `parse_primary()`: Identifier folding
- Before checking `AST_VAR_REF`, check if the identifier is a registered enum constant
- If found, emit `AST_INT_LITERAL` node with the constant's value
- Otherwise, fall through to `AST_VAR_REF` logic

## Required AST Changes

None. Enum constants fold to existing `AST_INT_LITERAL` nodes during parsing. No new node types or structural changes required.

## Required Code Generation Changes

None. Enum constants appear as `AST_INT_LITERAL` nodes in the AST, so codegen requires no special handling.

## Test Plan

Eight new test files:

### Valid Tests (5)
1. **test_enum_anonymous__2.c**
   - Anonymous enum: `enum { A, B, C };`
   - Returns `C` (expect 2)

2. **test_enum_named__2.c**
   - Named enum tag: `enum Color { RED, GREEN, BLUE };`
   - Declares: `enum Color c = BLUE;`
   - Returns `c` (expect 2)

3. **test_enum_explicit_values__11.c**
   - Enum with explicit values: `OK = 0, BAD = 10, WORSE`
   - Returns `WORSE` (expect 11, auto-increment from BAD)

4. **test_enum_char_values__66.c**
   - Enum with character constants: `A = 'A', B = 'B'`
   - Returns `B` (expect 66)

5. **test_enum_trailing_comma__2.c**
   - Enum with trailing comma: `enum { RED, GREEN, BLUE, };`
   - Returns `BLUE` (expect 2)

### Invalid Tests (3)
1. **test_enum_duplicate_error.c**
   - Duplicate enumerator: `enum { A, A };`
   - Expect error: duplicate enumerator

2. **test_enum_undefined_tag_error.c**
   - Undefined tag reference: `enum Missing x;`
   - Expect error: undefined enum tag

3. **test_enum_non_literal_error.c**
   - Non-literal explicit value: `A = 1 + 2`
   - Expect error: non-literal constant expression

## Documentation Updates

- **`docs/grammar.md`**: Update `<type_specifier>` production to include `<enum_specifier>`
- **`README.md`**: Update stage line (change "Through Stage 28-04" to "Through Stage 29") and add enum to capabilities list

## Known Ambiguities and Decisions

None identified. The `[", "]` in the grammar (enumerator list) clearly indicates an optional trailing comma, consistent with spec example 5. Enum constants are always integers; character constants like `'A'` are converted to their ASCII value at parse time.

## Implementation Order

1. Add `TOKEN_ENUM` to tokenizer, update lexer
2. Add `EnumConst` and `EnumTag` structs to parser
3. Implement `parse_enum_specifier()` with auto-increment and validation logic
4. Integrate `parse_enum_specifier()` into `parse_type_specifier()`
5. Add `TOKEN_ENUM` recognition to `parse_declaration_specifiers()`
6. Add standalone-declaration guards in `parse_statement()` and `parse_external_declaration()`
7. Implement identifier folding to `AST_INT_LITERAL` in `parse_primary()`
8. Implement and validate all eight test cases
9. Update `docs/grammar.md` and `README.md`
