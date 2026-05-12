# Stage 37: Incomplete Struct Declarations

## Summary

Stage 37 adds support for incomplete struct forms — forward declarations of struct tags before their body is defined. Two patterns must work:

1. `typedef struct ASTNode ASTNode;` followed by `struct ASTNode { int kind; ASTNode *left; ASTNode *right; };` — forward typedef then self-referential body definition.
2. `struct Lexer;` followed by `struct parser { struct Lexer *Lexer; int current; };` — opaque forward declaration used as a pointer-only field type.

## Required Tokenizer Changes

None.

## Required Parser Changes

In `src/parser.c`, function `parse_struct_specifier`:

**No-body else branch** (currently errors when `st->type == NULL`): Instead of erroring, create an incomplete placeholder type (`type_struct(0, 0)`) and assign it to `st->type`. Return it.

**Body-definition branch**: After computing `total_size`, `max_align`, and field descriptors — if `st->type` already exists and is incomplete (size == 0), update it in-place (mutate `size`, `alignment`, `fields`, `field_count`). This ensures that any typedef entries already pointing to the placeholder automatically reflect the complete layout. If `st->type` is NULL, create a new type as before.

## Required AST Changes

None.

## Required Code Generation Changes

None. Pointer fields to incomplete struct types work as-is since pointer size is always 8.

## Test Plan

- `test_incomplete_struct_self_ref__<N>.c` — Example 1: typedef forward decl, self-referential struct fields used as pointers; test returns an integer from main.
- `test_incomplete_struct_forward__<N>.c` — Example 2: opaque forward decl, pointer field in another struct; test returns an integer from main.

## Ambiguities / Decisions

- The spec examples lack `main()` functions; test cases will add trivial `main()` wrappers that return an integer.
- In-place mutation of the placeholder type (rather than pointer swapping) is the correct approach so typedef entries automatically see the completed layout.
- Grammar note: `struct Tag;` as a standalone incomplete declaration is now valid; the grammar.md notes section will be updated.
