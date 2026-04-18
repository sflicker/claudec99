# stage-09-03 Milestone: Multiple Functions & Parameter Lists

## Overview

Stage 9.3 extends the compiler to accept translation units containing
multiple function definitions and to parse function parameter lists. Only
function *definitions* are supported; forward declarations/prototypes and
function *calls* remain out of scope. Only `int` is supported for parameter
types and return types.

## Grammar Change

Before (stage 9.2):

```ebnf
<translation-unit>     ::= <external-declaration>
<function>             ::= "int" <identifier> "(" ")" <block_statement>
```

After (stage 9.3):

```ebnf
<translation-unit>       ::= <external-declaration> { <external-declaration> }
<function>               ::= "int" <identifier> "(" [ <parameter_list> ] ")" <block_statement>
<parameter_list>         ::= <parameter_declaration> { "," <parameter_declaration> }
<parameter_declaration>  ::= "int" <identifier>
```

`docs/grammar.md` was synced to this updated grammar.

## Semantic Rules Enforced

- Parameter names must be unique within a parameter list (parser-level).
- Function names must be unique within the translation unit (parser-level).
- Parameters share the outermost body scope: a body-level declaration that
  collides with a parameter name is rejected with `duplicate declaration`.
- Redeclaration of a parameter name in an *inner* block scope is allowed
  (standard shadowing).

## Code Changes

- `include/token.h` — added `TOKEN_COMMA`.
- `src/lexer.c` — emit `TOKEN_COMMA` for `,`.
- `include/ast.h` — added `AST_PARAM`.
- `src/ast_pretty_printer.c` — `AST_PARAM` prints as `Parameter: <name>`.
- `src/parser.c`
  - Added `parse_parameter_declaration` and `parse_parameter_list`.
  - `parse_function_decl` now accepts an optional parameter list.
  - `parse_translation_unit` loops external declarations until EOF.
  - Parser rejects duplicate parameter names and duplicate function names.
- `src/codegen.c`
  - `codegen_function` recognizes leading `AST_PARAM` children before the
    body block.
  - Frame prologue emitted whenever `params + locals > 0` (previously gated
    on locals only).
  - Parameters are copied from the SysV AMD64 argument registers
    (`edi, esi, edx, ecx, r8d, r9d`) into `[rbp - N]` slots, making them
    addressable through the existing local-variable path.
  - Functions with more than 6 parameters are rejected with a clear error.

## AST Shape

A `FunctionDecl` node now has this child layout:

```
FunctionDecl: <name>
  Parameter: <p1>
  Parameter: <p2>
  ...
  Block
    <statements>
```

Zero-parameter functions have exactly one child (the `Block`), so all
stage-9.2 AST snapshots remain valid.

## Test Changes

### Valid (`test/valid/`)
- `test_multi_func_main_first__42.c` — two functions, main first, helper unused.
- `test_func_with_params__42.c` — helper with two parameters, main returns 42.
- `test_func_six_params__42.c` — helper with the 6-parameter max.
- `test_param_shadowed_inner_scope__42.c` — inner block shadows a parameter.

### Invalid (`test/invalid/`)
- `test_invalid_4__duplicate_parameter.c` — `int bad(int a, int a)`.
- `test_invalid_5__duplicate_function.c` — two `int foo()` definitions.
- `test_invalid_6__duplicate_declaration.c` — body redeclares a parameter.

### Print-AST (`test/print_ast/`)
- `test_function_with_params.c` / `.expected` — snapshot with `Parameter:`
  children under `FunctionDecl`.

## Test Results

- `test/valid` — 78 / 78 pass (74 prior + 4 new).
- `test/invalid` — 6 / 6 pass (3 prior + 3 new).
- `test/print_ast` — 7 / 7 pass (6 prior + 1 new).
- Total: 91 / 91. No regressions.

## Constraints Honored

- No prototypes / forward declarations.
- No function calls.
- Only `int` parameters and `int` return types.
- Parameters are scoped to the function body and may be shadowed by
  inner blocks but not redeclared in the outer body scope.

## Decisions & Rationale

- **Parameter cap at 6.** SysV AMD64 passes the first six int arguments in
  registers; beyond that it spills to the stack. Since this stage does not
  support function calls, the parameter values are never set by a real
  caller, but the generated prologue copies from the SysV registers so the
  implementation is forward-compatible with a later "function calls" stage.
  Seven-plus parameters are rejected rather than silently truncated.
- **Main-first ordering in tests.** The link step uses `ld` without an
  explicit entry symbol, so execution begins at the start of the `.text`
  section. Multi-function tests place `main` first to keep this working.
- **Parser-level uniqueness checks.** Both duplicate-parameter and
  duplicate-function checks are done during parsing, so the AST handed to
  codegen is already valid with respect to these rules.
