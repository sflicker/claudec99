# Stage 72 Kickoff: Named Union Support

## Summary of the spec

Stage 72 adds support for named `union` types with complete layout computation and member access. Implementation includes:

- Union type definitions and declarations
- Layout computation (union size = largest member; all offsets = 0)
- Local and global variable declarations
- Member access via `.` (direct) and `->` (pointer)
- Whole-union assignment
- First-member initialization
- `sizeof` operator applied to unions and union fields
- Union fields nested inside structs
- Struct fields nested inside unions

Out of scope: anonymous unions, bit-fields, designated initializers, union by-value parameters/returns.

Two ambiguities in the spec are resolved by the implementation:
1. "Union inside struct" test defines `union U` but references `union Value`; corrected version uses `union U` throughout.
2. Union layout example missing closing `};` is added.
3. Typedef support for union types (e.g., `typedef union U U;`) is supported via existing typedef machinery without special effort.

## Required tokenizer changes

**File: `include/token.h`**
- Add `TOKEN_UNION` to `TokenType` enum

**File: `src/lexer.c`**
- Recognize "union" keyword and emit `TOKEN_UNION`
- Add "union" to `token_display_name()` switch

## Required parser changes

**File: `include/parser.h`**
- Add `UnionTag` struct (parallel to `StructTag`)
- Add union tag table to `Parser` struct

**File: `src/parser.c`**
- Implement `parse_union_specifier()` — parses union body, records fields
- Update `parse_type_specifier()` to handle `TOKEN_UNION`
- Update `parse_declaration_specifiers()` to handle `TYPE_UNION`
- Extend sizeof handling to accept `TYPE_UNION` (uses `compute_union_size()`)
- Update local declaration paths to support union variables
- Update global declaration paths to support union variables

## Required AST changes

No new AST nodes. Unions use existing `TypeKind` infrastructure:

- Add `TYPE_UNION` to `TypeKind` enum in `include/type.h`
- Implement `type_union()` function in `src/type.c`
- Update `type_kind_name()` to return "union" for `TYPE_UNION`
- `StructField` reused for union member metadata (all offsets = 0 for union members)
- `find_struct_field()` works unchanged for union field lookup

## Required code generation changes

**File: `src/codegen.c`**
- Update `emit_member_addr()` to handle `TYPE_UNION` member access (`.` operator)
  - For unions, member address = base address (offset always 0)
- Update `emit_arrow_addr()` to handle `TYPE_UNION` pointer member access (`->` operator)
  - For unions, member address = dereferenced pointer (offset always 0)
- Update `expr_result_type()` to return correct type for union member access
- Update `sizeof_type_of_expr()` to handle union expressions
- Update `compute_decl_bytes()` to compute union size (largest member)
- Update `local_var_type()` to return union type for union declarations
- Update local variable declaration to handle union initialization (first member only)
- Update global variable declaration to handle union initialization
- Update BSS emission for union globals to use `resb <union_size>` for correct allocation

**File: `src/ast_pretty_printer.c`**
- Update to display `TYPE_UNION` in type output

## Test plan

**8 new valid tests** under `test/valid/`:
- Basic union definition and member access
- Union pointer member access
- Union inside struct
- Struct inside union
- Union globals with initialization
- Whole-union assignment
- `sizeof` applied to unions
- Union field inside struct field access chain

**1 new invalid test** under `test/invalid/`:
- Incomplete union variable (reject `union U u;` when U has no body)

## Implementation order

1. **Token layer**: Add `TOKEN_UNION` to lexer
2. **Type system**: Add `TYPE_UNION`, implement `type_union()`
3. **Parser**: Implement `parse_union_specifier()`, wire `TYPE_UNION` throughout declaration/specifier paths
4. **Code generation**: Handle `TYPE_UNION` in member access, sizeof, declarations, assignment, global emission
5. **Tests**: Add valid and invalid test cases
6. **Documentation**: Update `docs/grammar.md` and `README.md`
