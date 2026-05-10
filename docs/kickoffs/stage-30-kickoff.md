# Stage 30 Kickoff: Struct Definition, Layout and Sizeof

## Summary

Stage 30 introduces named struct definitions with automatic layout and alignment computation, plus sizeof support for struct types and struct-typed variables. The compiler will compute natural-alignment layout, tracking struct size and field offsets, without yet supporting member access operators.

## Tokenizer Changes

**Token addition:**
- Add `TOKEN_STRUCT` for the "struct" keyword

**Implementation:**
- Update `include/token.h` to declare `TOKEN_STRUCT`
- Update `src/lexer.c` to recognize "struct" keyword and emit `TOKEN_STRUCT`
- Add display name for the token

## Parser Changes

**Grammar additions:**
```ebnf
<type_specifier> ::= <integer_type>
                   | <typedef_name>
                   | <enum_specifier>
                   | <struct_specifier>

<struct_specifier> ::= "struct" <identifier> "{" <struct_declaration_list "}"
                     | "struct" <identifier>

<struct_declaration_list> ::= <struct_declaration> { <struct_declaration> }

<struct_declaration> ::= <type_specifier> <struct_declarator_list> ";"

<struct_declarator_list> ::= <declarator> { "," <declarator> }
```

**Parser enhancements:**
- Add struct tag table and `PARSER_MAX_STRUCT_TAGS` constant to `include/parser.h`
- Implement `parse_struct_specifier()` function (similar to `parse_enum_specifier()`)
- Integrate struct_specifier into `type_specifier` parsing
- Support struct declarations in the declaration parsing flow
- Compute natural-alignment layout during struct definition: track field types, calculate offsets based on alignment requirements

**Scope:**
- Only named struct definitions and references
- Struct declarators use ordinary declarators (no bit-fields)
- Member access operators (`.` and `->`) out of scope

## Type System Changes

**Type kind addition:**
- Add `TYPE_STRUCT` to the `TypeKind` enum in `include/type.h`
- Add `struct_tag` field to union in Type structure to store struct name
- Add `size` and `alignment` fields to track computed layout

**Type constructors and helpers:**
- Implement `type_struct()` constructor to create struct types
- Update type helper functions (`is_struct_type()`, etc.) to recognize `TYPE_STRUCT`
- Ensure `sizeof` calculations account for struct types with their computed sizes

## Code Generation Changes

**Sizeof support:**
- Update `sizeof` expression codegen to handle `TYPE_STRUCT` for both type expressions and struct-typed variables
- Use computed struct size from type information

**Variable allocation:**
- Update local variable stack allocation to handle struct-typed variables
- Use struct size for stack frame adjustment

**Out of scope:**
- Member access codegen
- Struct initialization or assignment
- Struct parameters and return values

## Test Plan

**Test 1: Basic struct sizeof with type name**
- Define a struct `Point` with two `int` fields
- Return `sizeof(struct Point)` (expect 8 bytes)

**Test 2: Struct with mixed types and padding**
- Define a struct `Mixed` with `char` and `int` fields
- Return `sizeof(struct Mixed)` (result depends on padding)

**Test 3: Sizeof on struct-typed variable**
- Define a struct `Point` with two `int` fields
- Declare a struct-typed variable `p`
- Return `sizeof(p)` (expect 8 bytes)

## Implementation Order

1. **Tokenizer**: Add `TOKEN_STRUCT` token recognition
2. **Type system**: Add `TYPE_STRUCT` kind and struct type constructors
3. **Parser**: Implement struct specifier parsing and layout computation
4. **Code generation**: Handle `sizeof` for struct types
5. **Tests**: Create and verify test cases
6. **Documentation**: Update grammar and README

## Known Spec Issues to Fix

1. Grammar typo: `<struct_declaration_list` missing closing `>`
2. Test 2: missing trailing `;` on struct definition
3. Test 3: missing closing `}` on function body
