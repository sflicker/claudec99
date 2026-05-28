# Stage 75-03 Kickoff: Builtin Parsing and Semantic Recognition

## Summary of the Spec

Stage 75-03 adds parsing and semantic recognition of four GCC `__builtin_va_*` intrinsics that are produced when `<stdarg.h>` va_* macros expand. These builtins will be recognized as typed AST nodes and validated semantically, but code generation is a no-op (no ABI or runtime support yet).

**Spec note**: The spec file title says "stage 75-02" but the filename says "stage-75-03". The stage is 75-03. Also, several typos in the spec (`__buildin_` instead of `__builtin_`, "must ben" instead of "must be") will be corrected during implementation.

## Tokenizer Changes

None required. The builtin identifiers are ordinary C identifiers that will be tokenized as `TOKEN_IDENTIFIER`.

## Parser Changes

In `parse_primary()`, add special handling **before** general identifier resolution to recognize the four builtin forms:

1. `__builtin_va_start(expr, expr)` — exactly 2 arguments
2. `__builtin_va_end(expr)` — exactly 1 argument
3. `__builtin_va_copy(expr, expr)` — exactly 2 arguments
4. `__builtin_va_arg(expr, type_name)` — exactly 2 arguments; second is a type-name, not an expression

For `__builtin_va_start`, add semantic check: must be inside a variadic function. Track this with a `current_func_is_variadic` field in the Parser struct, set/restored during function definition parsing.

For `__builtin_va_arg`, the second argument parses as a type_name and the result type is stored in the AST node (`decl_type` and `full_type` fields).

## AST Changes

Add four new ASTNodeType values:
- `AST_BUILTIN_VA_START`
- `AST_BUILTIN_VA_END`
- `AST_BUILTIN_VA_COPY`
- `AST_BUILTIN_VA_ARG`

Existing ASTNode struct should already have the necessary fields (e.g., `children`, `num_children`, `decl_type`, `full_type`). For `__builtin_va_arg`, the result type is stored in `decl_type` and `full_type`.

## Code Generation Changes

At the end of `codegen_expression()`:
- For `AST_BUILTIN_VA_START`, `AST_BUILTIN_VA_END`, `AST_BUILTIN_VA_COPY`: set `result_type = TYPE_VOID` and return (no code emitted).
- For `AST_BUILTIN_VA_ARG`: emit `xor eax, eax` (dummy return value), set `result_type` from the node's `decl_type`.

## Test Plan

1. **Valid AST tests**: Two print_ast tests showing `va_start`/`va_end` and `va_arg` parsing correctly.
2. **Invalid tests**:
   - `va_start` called outside a variadic function → semantic error
   - `va_start` with 1 argument instead of 2 → semantic error
   - `va_arg` with identifier instead of type as second argument → semantic error (unknown type name)
3. Update version string from "00750200" to "00750300".

## Implementation Order

1. Add AST node types in `include/ast.h`
2. Add parser field in `include/parser.h`
3. Add builtin recognition in `src/parser.c` (parse_primary)
4. Add builtin cases in `src/ast_pretty_printer.c`
5. Add builtin cases in `src/codegen.c`
6. Update version in `src/version.c`
7. Create test files
8. Update `docs/grammar.md` with new builtin_expression production
9. Update README.md test counts and stage reference

## Ambiguities and Decisions

- **Variadic check scope**: The check applies only to `__builtin_va_start`. The other builtins are parsed but not validated for variadic context.
- **Type-name parsing**: `__builtin_va_arg`'s second argument parses via `parse_type_name()`, which validates against the current typedef table and symbol table.
- **No-op codegen**: Unlike standard C calls, builtins emit no argument evaluation code; they are purely recognized and validated.
