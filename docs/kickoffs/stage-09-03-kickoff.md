# stage-09-03 Kickoff: Multiple Functions & Parameter Lists

## Spec Summary

Stage 9.3 extends the compiler in two ways:

1. **Multiple function definitions per translation unit.** The grammar moves
   from `<translation-unit> ::= <external-declaration>` to
   `<translation-unit> ::= <external-declaration> { <external-declaration> }`.
2. **Function parameter lists.** `<function>` now accepts an optional
   `<parameter_list>` where each `<parameter_declaration>` is
   `"int" <identifier>` and items are separated by commas.

Only function *definitions* are supported. Forward declarations / prototypes
are out of scope. Function *calls* are out of scope. Only `int` types are
supported for parameters and return values.

## Semantic Rules (from spec)

- Parameter names must be unique within a function's parameter list.
- Function names must be unique within the translation unit.
- Parameters are in scope throughout the function body.
- Parameters behave like local variables for expression use.
- Redeclaring a parameter name in the same (outer) function scope is rejected.

## Changes from Stage 9.2

### Tokenizer
- Add `TOKEN_COMMA` to the `TokenType` enum.
- Emit `TOKEN_COMMA` from the lexer when it sees `,`.

### AST
- Add `AST_PARAM` node type.
- A `FunctionDecl` node's children become: zero or more `AST_PARAM` followed
  by the `AST_BLOCK` body.

### Pretty Printer
- Print `Parameter: <name>` for `AST_PARAM`.

### Parser
- Add `parse_parameter_list` / `parse_parameter_declaration`.
- Update `parse_function_decl` to accept an optional parameter list.
- Update `parse_translation_unit` to loop external declarations until EOF.
- Enforce unique parameter names within a function (parser-level).
- Enforce unique function names within the translation unit (parser-level).

### Code Generation
- Always emit the frame prologue when `params + locals > 0` (currently gated
  on locals only).
- At function entry, copy each parameter from its SysV AMD64 register
  (`edi`, `esi`, `edx`, `ecx`, `r8d`, `r9d`) into its reserved stack slot so
  the parameter is addressable by the same `[rbp - N]` mechanism used for
  locals.
- Cap parameters at 6 per function. Error if exceeded. Calls are not
  supported in this stage, so stack-passed arguments are moot.

### Documentation
- Sync `docs/grammar.md` to the spec grammar.

## Ambiguities / Decisions

1. **Parameter count cap.** Spec does not set an upper bound. SysV AMD64
   passes the first 6 int args in registers; anything beyond spills to the
   stack. Since no calls are supported this stage, we cap at 6 to keep the
   prologue simple and reject >6 with a clear error.
2. **Calling convention.** SysV AMD64 is used for parameter-slot
   initialization in the prologue. This is forward-compatible with a later
   stage that adds function calls; it costs nothing now.
3. **AST shape.** Parameters are stored as `AST_PARAM` children of the
   function *before* the `AST_BLOCK` body. Existing AST snapshots are
   unchanged for zero-parameter functions, so stage 9.2 expected outputs
   still match.
4. **Parameter/body name clash.** Parameters are added to the codegen
   symbol table before the body block opens, so an existing
   `scope_start`-based duplicate check naturally rejects a body-level
   `int <param>;` that collides with a parameter name.

## Planned File Updates

| File | Change |
|---|---|
| `include/token.h` | `TOKEN_COMMA` |
| `src/lexer.c` | Emit `TOKEN_COMMA` |
| `include/ast.h` | `AST_PARAM` |
| `src/ast_pretty_printer.c` | `Parameter:` label |
| `src/parser.c` | Param list + multi-decl translation unit + uniqueness checks |
| `src/codegen.c` | Always-on prologue + register-to-slot copy + 6-param cap |
| `docs/grammar.md` | Sync grammar |
| `test/valid/*.c` | Multi-function and parameter-use tests |
| `test/invalid/*.c` | Duplicate param / duplicate function / param-body clash |
| `test/print_ast/*` | Function-with-params snapshot |

## Test Plan (preview)

- **Valid:**
  - Function that declares parameters and uses them only inside dead code
    paths (since no calls happen, param values are undefined, but reading
    into locals inside an unreachable branch should still assemble/link).
  - Multi-function file where `main` ignores helpers.
  - `main` with parameters (treated as locals; runtime values undefined but
    guaranteed to assemble).
- **Invalid:**
  - Duplicate parameter name in one function.
  - Duplicate function name at translation-unit level.
  - Body redeclares a parameter: `int main(int a) { int a; return 0; }`.
- **print_ast:** New snapshot for a function with one or more parameters.

## Next Step

Pause for confirmation of this plan before starting tokenizer changes.
