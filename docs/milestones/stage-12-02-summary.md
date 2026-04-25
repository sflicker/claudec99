# Stage-12-02 Milestone Summary: Address-of and Dereference

Stage 12-02 adds the unary address-of (`&expr`) and dereference
(`*expr`) operators. Programs can now take a pointer to a variable,
store it in a pointer local, and read through it.

## Completed
- Added `TOKEN_AMPERSAND` and lexer support for a bare `&`.
- Added `AST_ADDR_OF` and `AST_DEREF` node types; reused
  `ASTNode.full_type` to carry the result Type for pointer-valued
  expressions.
- Extended `parse_unary` to accept `&` and `*` as unary prefixes;
  `&` is restricted to lvalues (`AST_VAR_REF`) at parse time.
- Extended `LocalVar` with `kind` and `full_type` so codegen can
  recover a local's pointer chain.
- Codegen:
  - `&x` emits `lea rax, [rbp - offset]` and produces a
    `pointer-to-T` result.
  - `*p` requires a pointer operand and loads through `[rax]` using
    the base type's width (`movsx eax, byte/word`, `mov eax, dword`,
    `mov rax, qword`).
  - Pointer initializers and assignments take the size-8 store path
    without sign-extension.
- Pretty printer renders `AddressOf:` and `Dereference:` nodes.
- Grammar updated to introduce `<unary_operator>` and add `*` and
  `&` to the unary operator set.

## Tests
- 3 new valid tests: basic address-of/deref (returns 7),
  assignment-through-pointer-variable (returns 3), nested
  dereference `**pp` (returns 9).
- 2 new invalid tests: dereferencing a non-pointer; address-of of
  a non-lvalue.
- 1 new print_ast test exercising `&` and `*`.
- All 224 tests pass (193 valid + 16 invalid + 15 print_ast). No
  regressions.
