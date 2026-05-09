# Stage 28-01 Kickoff: Simple typedef Names

## Summary of the Stage

Stage 28-01 adds support for simple `typedef` aliases for existing integer scalar types (char, short, int, long). Typedef names can be used as object declaration types, function return/parameter types, cast target types, and sizeof operands. Typedef declarations do not produce variables; they only register a type alias in the active scope.

Key constraint: Typedef names are scoped and must be managed through scope entry/exit points.

## Spec Issues Found

1. **Valid Test 2, line 86**: Typo — `tydedef char C;` should be `typedef char C;`
2. **Valid Test 3, lines 101-102**: `main()` has no return type; implicit int not supported — fix to `int main()`
3. **Valid Test 4, lines 110-111**: Missing `//` before "expect 4" in comment; closing `{` should be `}`
4. **Invalid Test 3, line 132**: Closing `{` should be `}`
5. **Minor wording, line 120**: "duplication typedef declarations" → "duplicate typedef declarations"

## Required Tokenizer Changes

- Add `TOKEN_TYPEDEF` token type to `include/token.h`
- Register "typedef" as keyword in lexer with appropriate token mapping
- Add display name for TOKEN_TYPEDEF in lexer

## Required Parser Changes

Add typedef name tracking to Parser struct in `include/parser.h`:
- TypedefEntry[64] storage for registered typedefs
- `typedef_count` to track active entries
- `scope_depth` to manage scope enter/exit

Parser implementation in `src/parser.c`:
- `typedef_register()` — add name to active scope, reject duplicates in same scope
- `typedef_lookup()` — check if identifier is registered typedef in active scope
- Extend `parse_type_specifier()` to accept typedef names as type specifiers
- Handle typedef declarations with storage class `SC_TYPEDEF` (new)
- Extend cast peek-ahead to recognize typedef names
- Extend sizeof peek-ahead to recognize typedef names
- Increment/decrement scope_depth on block entry/exit

## Required AST Changes

Add to `include/ast.h`:
- `SC_TYPEDEF = 4` storage class constant
- `AST_TYPEDEF_DECL` node type for typedef declarations

## Required Code Generation Changes

In `src/codegen.c`:
- No-op case for `AST_TYPEDEF_DECL` in codegen_statement (typedefs generate no code)

## Required Pretty Printer Changes

In `src/ast_pretty_printer.c`:
- Print case for `AST_TYPEDEF_DECL` node type

## Test Plan

Create test files in `test/valid/`:
- Basic typedef with int
- Typedef with char/short/long
- Typedef used in variable declaration
- Typedef in function return type
- Typedef in function parameter type
- Typedef in cast expression
- Typedef in sizeof operator
- Block-scope typedef shadowing file-scope typedef

Create test files in `test/invalid/`:
- Typedef declaration with initializer (reject)
- Duplicate typedef name in same scope (reject)
- Unknown identifier as type specifier (reject)
- Typedef with no type specifier (reject)

## Grammar Update

Update `docs/grammar.md`:

```ebnf
<storage_class_specifier> ::= "extern" | "static" | "typedef"

<type_specifier> ::= <integer_type> | <typedef_name>

<typedef_name> ::= <identifier>
```

Add semantic rule: An `<identifier>` is recognized as a `<typedef_name>` only if currently registered in the typedef table for the active scope.

## Implementation Order

1. Token: Add TOKEN_TYPEDEF to token.h
2. Lexer: Register typedef keyword with token mapping
3. AST: Add SC_TYPEDEF and AST_TYPEDEF_DECL
4. Parser: Add typedef table and scope management to Parser struct
5. Parser: Implement typedef_register and typedef_lookup
6. Parser: Extend parse_type_specifier to recognize typedef names
7. Parser: Add parse_typedef_decl handling
8. Parser: Extend cast and sizeof peek logic
9. Parser: Manage scope_depth on block entry/exit
10. Codegen: Add no-op case for AST_TYPEDEF_DECL
11. Pretty printer: Add print case for AST_TYPEDEF_DECL
12. Tests: Create valid and invalid test files
13. Grammar: Update docs/grammar.md

Pause after each step to validate incrementally.
