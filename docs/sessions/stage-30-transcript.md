# stage-30 Transcript: Struct Definition, Layout, and sizeof

## Summary

Stage 30 introduces struct type definitions with natural-alignment field layout and sizeof support. Named structs can be declared with multiple fields and then referenced in variable declarations; the compiler computes struct field offsets using standard C ABI natural alignment (each field aligned to its own size, struct padded to max field alignment). The sizeof operator extends to support struct types in both type and expression forms, returning the complete struct byte size as a compile-time constant. Member access via "." and "->" operators is out of scope.

## Changes Made

### Step 1: Tokenizer

- Added TOKEN_STRUCT to the TokenType enum in include/token.h
- Updated lexer.c to recognize and emit TOKEN_STRUCT for the `struct` keyword

### Step 2: Type System

- Added TYPE_STRUCT to the TypeKind enum in include/type.h
- Implemented type_struct(uint64_t size) constructor that creates a struct type with a given byte size
- Struct types store size information analogously to TYPE_ARRAY

### Step 3: Parser Setup

- Added StructTag struct to track named structs: { char name[256], Type type }
- Added PARSER_MAX_STRUCT_TAGS (32) constant and struct_tags table to Parser struct in include/parser.h
- Added lookup_struct_tag(Parser *p, const char *name) helper to retrieve struct type by name

### Step 4: Struct Grammar and Parsing

- Added parse_struct_specifier() function that:
  - Parses struct keyword followed by optional tag name and optional body
  - Computes natural-alignment field layout: each field padded to its own alignment, struct size rounded to max field alignment
  - Registers named structs in the struct_tags table
  - Returns a Type representing the struct
- Added forward declarations for parse_type_specifier and parse_declarator before parse_struct_specifier (needed for recursive use in struct body parsing)
- Updated parse_type_specifier to recognize TOKEN_STRUCT and call parse_struct_specifier
- Updated parse_statement, parse_external_declaration, and parse_declaration_specifiers to recognize struct definitions

### Step 5: Code Generation

- Extended sizeof handling in codegen.c:
  - Updated AST_SIZEOF_TYPE to compute struct size via type_kind_bytes()
  - Updated AST_SIZEOF_EXPR to compute struct type size for expression operands
- Updated local variable allocation (alloc_local) to handle TYPE_STRUCT with proper byte count
- Updated compute_decl_bytes() to return byte count for struct types
- Updated local_var_type() to recognize TYPE_STRUCT fields in variable lists

### Step 6: Grammar Documentation

- Updated docs/grammar.md with struct_specifier rule (named or referenced struct)
- Added struct_declaration_list and struct_declaration rules (type specifier followed by declarators)
- Added struct_declarator_list rule (comma-separated declarators)

### Step 7: Tests

- Added test_struct_sizeof_point__8.c: sizeof(struct Point) with two int fields, expects 8 bytes
- Added test_struct_sizeof_mixed__8.c: sizeof(struct Mixed) with char and int fields, natural alignment, expects 8 bytes
- Added test_struct_sizeof_var__8.c: sizeof(p) where p is a struct Point variable, expects 8 bytes

## Final Results

Build succeeds. All 739 tests pass (736 existing + 3 new). No regressions.

## Session Flow

1. Read spec and supporting templates
2. Reviewed relevant codegen, parser, and type system files
3. Planned implementation: tokenizer → type system → parser → codegen
4. Implemented TOKEN_STRUCT and type_struct() constructor
5. Implemented parse_struct_specifier with natural-alignment field layout
6. Extended sizeof codegen to handle struct types
7. Added three test cases covering basic struct sizeof scenarios
8. Built and verified all tests pass without regression
9. Generated milestone summary, transcript, and README update

## Notes

- Natural alignment is the standard C ABI approach: each field gets the alignment of its type (e.g., int requires 4-byte alignment), and the struct overall is padded to the alignment of its largest field.
- Struct types mirror the TYPE_ARRAY pattern in codegen: size lives on full_type, type_kind_bytes() returns 0 for TYPE_STRUCT (size is accessed from the type directly).
- The spec contained three typos corrected during implementation: grammar rule missing ">", test 2 missing ";", test 3 missing "}".
- Member access (dot and arrow operators), anonymous structs, initializers, and parameters/return values are out of scope for this stage and reserved for future stages.
