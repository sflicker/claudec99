# Stage-12-01 Milestone: Pointer Types

The compiler's type system now represents pointer types and the parser
accepts pointer variable declarations using the existing integer base
types — `char *`, `short *`, `int *`, `long *` — with arbitrary
nesting (`int **p`, `int ***t`). Pointer locals reserve 8 bytes on
x86-64. Address-of, dereference, pointer arithmetic, comparisons,
arrays, and pointer-typed parameters/returns remain out of scope.

## What was completed
- Type system (include/type.h, src/type.c): added `TYPE_POINTER` to
  the `TypeKind` enum and a `Type *base` field to the `Type` struct.
  Added a `type_pointer(Type *base)` factory that heap-allocates a
  pointer Type (size 8, alignment 8). `type_kind_name` returns
  `"pointer"` and `type_is_integer` returns 0 for pointers.
- AST (include/ast.h): added a `Type *full_type` slot on `ASTNode`,
  populated for pointer declarations with the head of the pointer
  Type chain.
- Parser (src/parser.c): the declaration branch of `parse_statement`
  now consumes zero or more `*` tokens after the integer base type,
  wrapping each level via `type_pointer`. When at least one `*` is
  consumed, `decl_type = TYPE_POINTER` and `full_type` is set.
- Code generator (src/codegen.c): `type_kind_bytes(TYPE_POINTER)`
  returns 8, so pointer locals reserve a full 8-byte stack slot. No
  other codegen paths needed updates because the operators that
  produce/consume pointer values are out of scope.
- Pretty printer (src/ast_pretty_printer.c): pointer declarations
  print using standard C notation by recursively walking the Type
  chain (`int *p`, `int **q`, `int ***t`).
- Seven new valid tests covering single-level pointer declarations
  for each integer base type, double- and triple-level nesting, and
  a mixed pointer/integer local file.
- One new print_ast test (`test_stage_12_01_pointer_types`)
  exercising every pointer form.
- `docs/grammar.md` updated: `<declaration>` now references a new
  `<type> ::= <integer_type> { "*" }` non-terminal.

## Scope limits (per spec)
No address-of, no dereference, no pointer arithmetic or comparisons,
no arrays, and no pointer-typed function parameters or return types.
Pointer initializers are accepted only insofar as the existing
expression/type system already parses them.

## Test results
All 218 tests pass (190 valid + 14 invalid + 14 print_ast),
including the 7 new pointer-decl tests and the 1 new print_ast
test. No regressions.
