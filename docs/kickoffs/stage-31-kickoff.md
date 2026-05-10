# Stage 31 Kickoff: Struct Member Access

## Summary

Stage 31 adds struct member access via the `.` (dot) and `->` (arrow) operators, supporting both lvalue and rvalue contexts. Building on Stage 30's struct definitions and layout computation, this stage enables reading and writing struct fields.

## Tokenizer Changes

**Token additions:**
- Add `TOKEN_DOT` for the `.` operator
- Add `TOKEN_ARROW` for the `->` operator

**Implementation:**
- Update `include/token.h` to declare `TOKEN_DOT` and `TOKEN_ARROW`
- Update `src/lexer.c` to recognize `.` as `TOKEN_DOT` in the single-character branch
- Update `src/lexer.c` to recognize `->` as `TOKEN_ARROW` in the `-` branch (two-character operator)
- Add display names for both tokens in `token_display_name()`

## Type System Changes

**StructField struct:**
- Add `StructField` struct to `include/type.h`:
  ```c
  typedef struct {
      char name[256];
      int offset;
      TypeKind kind;
      struct Type *full_type;
  } StructField;
  ```

**Type struct enhancements:**
- Add `fields` (StructField*) and `field_count` (int) to the Type struct for `TYPE_STRUCT` nodes
- Note: `type_struct()` already uses calloc, so fields default to NULL and field_count to 0; parser fills them in

## AST Changes

**New node types:**
- Add `AST_MEMBER_ACCESS` for `.` operator access
- Add `AST_ARROW_ACCESS` for `->` operator access

**Node structure:**
- Both nodes: `child[0]` = base expression (AST_VAR_REF or another access expression), `value` = field name (char*)

## Parser Changes

**Struct parsing enhancements:**
- Update `parse_struct_specifier()` to record each field's name, offset, and type in a local buffer during body parsing
- After creating the struct Type via `type_struct()`, calloc and populate `type->fields` with the collected field information

**Postfix expression parsing:**
- Update `parse_postfix()` to add `TOKEN_DOT` and `TOKEN_ARROW` to the existing while loop for postfix operators
- Create `AST_MEMBER_ACCESS` when TOKEN_DOT is encountered
- Create `AST_ARROW_ACCESS` when TOKEN_ARROW is encountered
- Consume the field name as an identifier token

**Lvalue handling:**
- Update `parse_assignment_expression()` to add `AST_MEMBER_ACCESS` and `AST_ARROW_ACCESS` to the set of valid lvalue types

**Grammar note:**
- The spec places `.` and `->` outside the `{ }` repetition group; implement them inside the postfix loop to match C semantics where member access is a postfix operator.

## Code Generation Changes

**Helper functions:**
- `find_struct_field(Type *st, const char *name)` → returns StructField*
- `emit_member_addr(cg, node)` → StructField*: computes address of field in rax for dot access; for struct at rbp-offset, field addr = rbp-(offset - field_offset)
- `emit_arrow_addr(cg, node)` → StructField*: loads pointer from base expression, adds field offset

**Expression codegen:**
- Update `codegen_expression()` to handle `AST_MEMBER_ACCESS` and `AST_ARROW_ACCESS` as rvalues (load value from field)

**Assignment codegen:**
- Update `AST_ASSIGNMENT` handling to add paths for `child[0] = AST_MEMBER_ACCESS` (store to field)
- Update `AST_ASSIGNMENT` handling to add paths for `child[0] = AST_ARROW_ACCESS` (store to field via pointer)

**Type support:**
- Update `sizeof_type_of_expr()` to handle new node types (return type of accessed field)
- Update `expr_result_type()` to handle new node types (return type of accessed field)

## Test Plan

**Core Tests:**
- `test_struct_dot_basic__7.c`: Define struct Point with x, y; set p.x=3, p.y=4; return p.x+p.y (expect 7)
- `test_struct_arrow_basic__30.c`: Define struct Point with x, y; create pointer q=&p; set q->x=10, q->y=20; return q->x+q->y (expect 30)
- `test_struct_dot_padding__15.c`: Define struct Mixed with char c, int i; set s.c=5, s.i=10; return s.c+s.i (expect 15)

**Invalid Tests:**
- `test_invalid_struct_dot_nonmember__...`: Attempt dot operator on int variable (should fail semantic check)
- `test_invalid_struct_dot_unknown_field__...`: Access non-existent field via dot operator (should fail semantic check)
- `test_invalid_struct_arrow_nonptr__...`: Attempt arrow operator on non-pointer struct (should fail semantic check)

## Known Spec Issues to Fix

1. Grammar formatting: `.` and `->` appear outside the `{ }` repetition group; implement inside the postfix loop (correct C behavior)
2. Core test 2: missing `;` after struct body — use `};`
3. Invalid test 1: `main()` lacks return type — use `int main()`
4. Invalid test 3: uses `struct p;` which fails at parse time (unknown struct tag); adjust to test `->` on non-pointer struct instead

## Implementation Order

1. **Tokenizer**: Add TOKEN_DOT and TOKEN_ARROW recognition
2. **Type system**: Add StructField struct and fields/field_count to Type
3. **AST**: Add AST_MEMBER_ACCESS and AST_ARROW_ACCESS node types
4. **Parser**: Implement struct field recording, postfix operator parsing, and lvalue recognition
5. **Code generation**: Implement helpers and codegen paths for member access
6. **Tests**: Create and verify test cases
7. **Documentation**: Update grammar.md with corrected postfix_expression rule
