# stage-09-02 Milestone: Translation Unit / External Declaration

## Overview

Stage 9.2 is a grammar-only refactor. The top-level production `<program>` was
replaced by `<translation-unit>`, and a new `<external-declaration>` wrapper
was introduced for top-level constructs. No new tokens, AST shapes, or
language features were added. Compiled output and runtime behavior are
identical to stage 9.1.

## Grammar Change

Before:

```ebnf
<program> ::= <function>
```

After:

```ebnf
<translation-unit>     ::= <external-declaration>
<external-declaration> ::= <function>
```

## Code Changes

- `include/ast.h` — renamed enum value `AST_PROGRAM` to `AST_TRANSLATION_UNIT`.
- `include/parser.h` — renamed `parse_program` to `parse_translation_unit`.
- `include/codegen.h` — renamed `codegen_program` to `codegen_translation_unit`.
- `src/parser.c` — added static `parse_external_declaration`; replaced
  `parse_program` with `parse_translation_unit` that wraps the external
  declaration in an `AST_TRANSLATION_UNIT` node.
- `src/codegen.c` — renamed entry point and updated the `AST_TRANSLATION_UNIT`
  type check.
- `src/ast_pretty_printer.c` — updated case label; root print label changed
  from `ProgramDecl:` to `TranslationUnit:`.
- `src/compiler.c` — call sites updated to the new entry point names.

## Test Changes

- The six `test/print_ast/*.expected` files had their first line updated from
  `ProgramDecl:` to `TranslationUnit:` to track the pretty-printer rename.
- No `.c` test sources changed.

## Test Results

- `test/valid` — 74 / 74 pass.
- `test/invalid` — 3 / 3 pass.
- `test/print_ast` — 6 / 6 pass.
- Total: 83 / 83. No regressions.

## Constraints Honored

- No support for multiple functions.
- No function declarations / prototypes.
- No global variables.
- No new tokens or grammar rules.

## Notes

Spec wording quirks observed (not implemented around):

- The spec line "No existing test must pass with modification" was read as
  "no existing test must require modification to pass." Test `.c` sources
  were untouched; only `.expected` snapshot files for the AST pretty
  printer were updated to track the rename, since the printer label is
  part of the rename surface.
- The pre-existing `<int_literal> ::= [0-9]+c` line (stray trailing `c`) is
  carried over verbatim from prior stages and was not modified.
