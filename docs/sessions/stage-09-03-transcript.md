# stage-09-03 Transcript: Multiple Functions and Parameter Lists

## Summary

Stage 9.3 adds function parameter lists and support for multiple function
definitions per translation unit. The grammar's top rule now permits one or
more `<external-declaration>`, and `<function>` accepts an optional
parameter list of `int <identifier>` items separated by commas. Parameters
are stored in the AST as `AST_PARAM` children preceding the block body, and
the code generator copies the SysV AMD64 argument registers into `[rbp - N]`
slots so parameters are reachable through the same local-variable path used
by `int` declarations.

Only function definitions are supported — forward declarations, prototypes,
and function calls remain out of scope. Only `int` is supported for both
parameter types and return types. Parameters share the outermost body scope,
so body-level redeclaration is rejected while inner-block shadowing is
allowed. The compiler enforces unique parameter names within a function and
unique function names within the translation unit, and rejects functions
with more than six parameters (the SysV AMD64 register set for int args).

## Plan

Implementation order: tokenizer → AST → parser → code generator → tests →
grammar doc. Paused after the kickoff for plan confirmation, then
implemented in small steps, running the full test suite after each
significant change.

## Changes Made

### Step 1: Tokenizer

- Added `TOKEN_COMMA` to `TokenType` in `include/token.h`.
- Emitted `TOKEN_COMMA` for the `,` character in `src/lexer.c`.

### Step 2: AST

- Added `AST_PARAM` to the `ASTNodeType` enum in `include/ast.h`.
- Added a `Parameter: <name>` case to `ast_pretty_print` in
  `src/ast_pretty_printer.c`.

### Step 3: Parser

- Added `parse_parameter_declaration` returning an `AST_PARAM` node.
- Added `parse_parameter_list` that appends parameters to the function node
  and rejects duplicate parameter names with
  `duplicate parameter '<name>' in function '<fn>'`.
- Updated `parse_function_decl` to accept an optional parameter list
  between `(` and `)`.
- Updated `parse_translation_unit` to loop external declarations until EOF
  and reject duplicates with
  `duplicate function '<name>' in translation unit`.

### Step 4: Code Generator

- `codegen_function` now identifies leading `AST_PARAM` children and takes
  the body block as the child immediately after them.
- Functions with more than six parameters are rejected at codegen.
- Frame prologue is now emitted whenever `params + locals > 0`.
- Each parameter is registered as a local via `codegen_add_var` and the
  matching SysV AMD64 register is stored into its `[rbp - N]` slot at
  function entry:

```asm
mov [rbp - 4], edi
mov [rbp - 8], esi
```

- Parameters stay in the outermost body scope so body-level redeclaration
  trips the existing `scope_start` duplicate check.

### Step 5: Tests

- `test/valid/test_multi_func_main_first__42.c` — two functions, main first.
- `test/valid/test_func_with_params__42.c` — helper `add(int, int)`.
- `test/valid/test_func_six_params__42.c` — helper with max 6 parameters.
- `test/valid/test_param_shadowed_inner_scope__42.c` — inner block shadows
  a parameter.
- `test/invalid/test_invalid_4__duplicate_parameter.c`.
- `test/invalid/test_invalid_5__duplicate_function.c`.
- `test/invalid/test_invalid_6__duplicate_declaration.c` — body redeclares
  a parameter.
- `test/print_ast/test_function_with_params.c` and `.expected` —
  `FunctionDecl` with `Parameter:` children.

### Step 6: Documentation

- `docs/grammar.md` updated with
  `<translation-unit>`, `<function>`, `<parameter_list>`, and
  `<parameter_declaration>` productions from the stage spec.

## Final Results

All 91 tests pass (78 valid + 6 invalid + 7 print_ast). No regressions from
stage 9.2 (previous total: 83).

## Session Flow

1. Read the spec and stage 9.2 milestone summary.
2. Reviewed tokenizer, parser, AST, codegen, and existing tests.
3. Produced kickoff summary and planned-changes outline; paused for
   confirmation.
4. Implemented tokenizer, AST, parser, and codegen changes in sequence,
   building after each.
5. Added new valid, invalid, and print_ast tests; ran all test suites.
6. Caught a scoping bug — parameters were placed outside the body scope —
   and corrected by leaving `scope_start` at 0 so body-level duplicates are
   rejected while inner-block shadowing still works.
7. Updated `docs/grammar.md` and produced the milestone summary and this
   transcript.

## Notes

- Linker entry point: `ld` is invoked without an explicit entry, so
  execution starts at the beginning of `.text`. Multi-function tests place
  `main` first to keep this working. No change was made to the link step.
- SysV AMD64 convention was chosen for parameter-slot initialization even
  though no function calls occur in this stage; the prologue register
  copies are dead code now but will be correct when function calls are
  added in a later stage.
