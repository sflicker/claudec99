# Stage-12-01 Kickoff: Pointer Types

## Spec Summary
Stage 12-01 introduces internal representation for pointer types in the
compiler. The type system gains a pointer kind that records its
referenced base type, and the parser accepts pointer variable
declarations using the existing integer base types — `char *`,
`short *`, `int *`, `long *` — with arbitrary nesting (`int **p`,
`int ***t`). Pointer locals reserve 8 bytes on x86-64. Pointer
initializers are allowed only insofar as the existing expression/type
system already supports them.

## Out of Scope (per spec)
- Address-of operator `&x`
- Dereference operator `*p`
- Pointer assignment through dereference `*p = value`
- Pointer arithmetic
- Pointer comparisons
- Arrays
- Function parameters or return types involving pointers

## What changes from prior stages
Currently `TypeKind` only models four integer kinds and `ASTNode`
carries only a `TypeKind` for declared type. The parser accepts a
single integer keyword as the declaration type. After this stage:
- `TypeKind` gains `TYPE_POINTER`; `Type` gains a `Type *base` field.
- A new `type_pointer(Type *base)` factory builds nested pointer
  chains (heap-allocated).
- `ASTNode` gains a `Type *full_type` slot used by pointer
  declarations to carry the full pointer chain.
- The declaration parser consumes zero-or-more `*` tokens after the
  integer base type and wraps the type accordingly.
- Codegen treats `TYPE_POINTER` as 8 bytes for stack allocation;
  integer paths are unchanged.
- The pretty printer prints pointer types using standard C notation
  (`int *p`, `int **s`, `int ***t`).

## Planned Changes
1. **Tokenizer** — no change. `*` already exists as `TOKEN_STAR`.
2. **Type system** (`include/type.h`, `src/type.c`)
   - Add `TYPE_POINTER` to `TypeKind`.
   - Add `Type *base` to the `Type` struct.
   - Add `Type *type_pointer(Type *base)` (heap-allocated; size=8,
     alignment=8, is_signed=0).
   - Update `type_kind_name` (return `"pointer"` for `TYPE_POINTER`)
     and `type_is_integer` (return 0 for `TYPE_POINTER`).
3. **AST** (`include/ast.h`)
   - Add `struct Type *full_type` to `ASTNode`. NULL for non-pointer
     declarations.
4. **Parser** (`src/parser.c`)
   - In the declaration branch of `parse_statement`: after the
     integer keyword, consume zero-or-more `TOKEN_STAR` tokens,
     wrapping a `Type *` chain. If at least one `*` is seen, set
     `decl_type = TYPE_POINTER` and `full_type = <chain>`.
   - Function parameters and return types remain integer-only.
5. **Code generator** (`src/codegen.c`)
   - `type_kind_bytes(TYPE_POINTER)` returns 8.
6. **Pretty printer** (`src/ast_pretty_printer.c`)
   - For `AST_DECLARATION` with `decl_type == TYPE_POINTER`, recursively
     print the base type followed by the appropriate number of `*`
     characters before the variable name.
7. **Tests**
   - `test/valid/`: minimal pointer-decl tests for `char *`,
     `short *`, `int *`, `long *`, plus `int **` and `int ***`. Each
     returns `0`.
   - `test/print_ast/`: add `test_stage_12_01_pointer_types.c` and
     matching `.expected` covering pointer declarations to multiple
     levels.
8. **Documentation**
   - Update `docs/grammar.md`: `<declaration>` and a new `<type>`
     non-terminal allowing optional `*` repetition.

## Open Spec Notes
- Spec filename has a duplicated `stage` token
  (`...stage-stage-12-01...`). `STAGE_LABEL` derived as `stage-12-01`
  from the second occurrence, where the numeric tokens follow.
- Pretty-printer format for pointer declarations is not specified;
  using standard C notation (`int *p`, `int **s`, `int ***t`).
- Initializer semantics are loose: the parser accepts whatever the
  existing expression system parses, with no new pointer-vs-integer
  type checking. With no `&` operator yet, the only practical
  initializer is an integer expression.
