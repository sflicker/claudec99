# stage-14-02 Transcript: String Literal AST Node

## Summary

Stage 14-02 wires the `TOKEN_STRING_LITERAL` token from stage 14-01
into the parser and AST. A new `AST_STRING_LITERAL` node represents a
string literal as a `<primary_expression>`; its body bytes are kept
on the node's `value` field and its byte length is preserved via
`full_type = type_array(type_char(), byte_length + 1)`, matching the
spec's `char[N+1]` type rule (the trailing slot covers the implicit
NUL terminator).

The AST pretty-printer renders the node as
`StringLiteral: "<bytes>" (length N)`. Two new `print_ast` tests
cover the basic and empty-string cases. No lexer, codegen, or token
changes were required — emitting strings, taking their addresses,
escape sequences, and runtime behavior remain out of scope.

## Plan

After reading the spec, the planning phase decided to:

- Reuse the existing `ASTNode.full_type` slot rather than add a new
  `length` field. Storing the literal as `char[N+1]` preserves the
  byte length and aligns with the spec's type rule.
- Render the new node as `StringLiteral: "<bytes>" (length N)` so
  the preserved length is visible and empty strings print
  unambiguously.
- Skip codegen entirely — `codegen_expression` already falls through
  to a no-op for unknown node kinds, and the new tests use
  `--print-ast`.

## Changes Made

### Step 1: AST

- Added `AST_STRING_LITERAL` to `ASTNodeType` in `include/ast.h`.

### Step 2: Parser

- Extended `parse_primary` in `src/parser.c` with a
  `TOKEN_STRING_LITERAL` branch that consumes the token, builds
  an `AST_STRING_LITERAL` node, copies the byte payload via
  `ast_new`, sets `decl_type = TYPE_ARRAY`, and assigns
  `full_type = type_array(type_char(), token.length + 1)`.

### Step 3: Pretty Printer

- Added a `AST_STRING_LITERAL` case in `src/ast_pretty_printer.c`
  that prints `StringLiteral: "<bytes>" (length N)`. The byte
  length is recovered as `full_type->length - 1`.

### Step 4: Tests

- Added `test/print_ast/test_stage_14_02_string_literal_basic.c`
  exercising `return "hi";` (and matching `.expected`).
- Added `test/print_ast/test_stage_14_02_string_literal_empty.c`
  exercising `return "";` (and matching `.expected`).

### Step 5: Grammar

- Updated `docs/grammar.md`: added a `<string_literal>` production
  and the new `<primary_expression>` alternative.

## Final Results

Build clean. All 369 tests pass (232 valid + 44 invalid + 21 print-AST
+ 72 print-tokens). Two new print-AST tests added; no regressions.

## Session Flow

1. Read spec and supporting files (`docs/stages/...stage-14-02...`,
   skill notes, transcript format).
2. Reviewed lexer, parser, AST, pretty-printer, and the existing
   stage 14-01 milestone to understand the token's shape.
3. Produced kickoff summary covering decisions and planned changes;
   paused for confirmation.
4. Implemented the AST enum addition, parser branch, and
   pretty-printer case.
5. Built and ran the compiler against the two new test programs to
   capture actual output, then wrote the matching `.expected` files.
6. Ran the aggregate test runner (`test/run_all_tests.sh`) and
   verified 369/369 pass with no regressions.
7. Updated `docs/grammar.md` with the new productions.
8. Wrote milestone summary and this transcript.
