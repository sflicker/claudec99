# Stage 138 Kickoff: Support `auto` and `register` Storage-Class Specifiers

## Spec Summary

CC99-011 and CC99-012 identify that the parser rejects the C99 storage-class specifiers `auto` and `register`.

**CC99-011:** Explicit `auto` declarations at block scope are rejected. The parser expects a type immediately after the semicolon, treating `auto` as an unknown type name. The spec requires `auto` to be accepted for block-scope object declarations only, with semantics identical to default automatic storage.

**CC99-012:** `register` declarations and parameters are rejected. Like `auto`, it's treated as an unknown type name. The spec requires `register` to be accepted for block-scope object declarations and function parameters, treated as automatic storage for allocation purposes, with the restriction that address-of (`&`) on a `register` object must be rejected.

Both specifiers must be rejected at file scope.

Test programs (both returning 27) validate these features against GCC C99 semantics.

## Tokenizer Changes

**include/token.h:**
- Add `TOKEN_AUTO`
- Add `TOKEN_REGISTER`

**src/lexer.c:**
- Add keyword recognition for "auto" returning `TOKEN_AUTO`
- Add keyword recognition for "register" returning `TOKEN_REGISTER`
- Add display names for both tokens in token display logic

## Parser Changes

**src/parser.c:**

`parse_declaration_specifiers`:
- Handle `TOKEN_AUTO` and `TOKEN_REGISTER` as storage-class specifiers
- Set the corresponding StorageClass in the parsed declaration specifiers

`parse_statement` (block-scope declarations):
- Extract storage-class from declaration specifiers
- Accept `TOKEN_AUTO` and `TOKEN_REGISTER` at block scope

File-scope rejection:
- `parse_external_declaration` or declaration parsing must reject `TOKEN_AUTO` and `TOKEN_REGISTER` at file scope with diagnostic error

Function parameter handling:
- `parse_parameter_declaration` must accept `TOKEN_REGISTER` for parameters
- Set appropriate storage class in the parsed parameter

## AST Changes

**include/ast.h:**
- Add `SC_AUTO = 8` to StorageClass enum
- Add `SC_REGISTER = 16` to StorageClass enum

## Code Generation Changes

**include/codegen.h:**
- Add `is_register` field (bool) to LocalVar struct to track `register` declarations

**src/codegen.c:**
- In `codegen_add_var`: Initialize `is_register` to false for all variables
- When processing `SC_REGISTER` declarations, set `is_register = true`
- In `AST_ADDR_OF` handler: Reject with diagnostic if attempting address-of on a register variable
- No behavioral difference for code generation between default automatic and `auto` or `register` (all treated as normal local variables)

**src/ast_pretty_printer.c:**
- Add display for `SC_AUTO` in storage-class display logic
- Add display for `SC_REGISTER` in storage-class display logic

**src/version.c:**
- Bump `VERSION_STAGE` to `"13800000"`

## Test Plan

**Valid tests to add:**
- Explicit `auto int x;` at block scope
- `auto` arrays at block scope
- Initialized `auto` objects
- Explicit `auto int` in nested scopes
- `register int x;` at block scope
- `register` arrays at block scope
- Initialized `register` objects
- `register` function parameters
- `register` parameters with multiple declarations in same function

**Invalid tests:**
- File-scope `auto` declaration (diagnostic required)
- File-scope `register` declaration (diagnostic required)
- Address-of `register` variable in block scope (diagnostic required)
- `register` with array bounds (valid semantically but commonly restricted)

## Implementation Order

1. Add tokens to include/token.h
2. Update lexer.c for keyword recognition and display names
3. Add storage classes to include/ast.h
4. Update parser.c to parse and validate storage classes
5. Add is_register field to include/codegen.h
6. Update src/codegen.c to handle is_register and reject &register
7. Update src/ast_pretty_printer.c for display
8. Bump version to 13800000
9. Add test cases
10. Verify self-compilation

## Decisions

- No semantic difference between `auto` and default automatic storage; `auto` is syntactic only
- `register` is treated like `auto` for allocation but tracked with `is_register` flag to enable address-of diagnostic
- Both specifiers rejected at file scope with clear diagnostic messages
- Implementation follows existing storage-class patterns (extern, static)
