# Stage-11-01 Kickoff: Minimal Type System

Spec: `docs/stages/ClaudeC99-spec-stage-11-01-minimal-type-system.md`

## Goal

Introduce Lexer / Parser / AST support for the integer-type keywords
`char`, `short`, `int`, `long` in **local variable declarations only**.
Add a minimal (mostly stub) type-system module that later stages can
extend. Codegen behaviour is unchanged this stage — all four types are
still emitted as 32-bit `int`.

## What changes from the previous stage

- Previously only `int` could start a declaration.
- After this stage any of `char | short | int | long` starts a
  declaration, the declared type is carried on the AST node, the AST
  printer surfaces it, and a `Type` module exists (with stubbed
  helpers).
- Function declarations, definitions, parameters, and return types
  remain restricted to `int`.

## Grammar

```ebnf
<declaration>  ::= <integer-type> <identifier> [ "=" <expression> ] ";"
<integer-type> ::= "char" | "short" | "int" | "long"
```

## Notes / Spec observations

- The "example type system code" in the spec is wrapped with single
  backticks rather than fenced blocks — rendering quirk only, no
  semantic ambiguity.
- `is_signed` is listed on the example `Type` struct even though signed
  / unsigned variants are explicitly out of scope. The field is kept
  (all four types treated as signed) but no keyword support is added.
- Stub helpers may return conventional size/alignment values
  (char=1, short=2, int=4, long=8). Codegen does not read them.
- AST storage: a `TypeKind decl_type` field is added to `ASTNode`.
  Only `AST_DECLARATION` uses it at this stage.
- Printer format proposed: `VariableDeclaration: <type> <name>` (e.g.
  `VariableDeclaration: int a`). Existing print-AST expectation files
  updated accordingly.

## Planned Changes

| File | Change |
|---|---|
| `include/type.h` (new) | `TypeKind`, `Type`, stub prototypes. |
| `src/type.c` (new) | Stub implementations. |
| `CMakeLists.txt` | Add `src/type.c`. |
| `include/token.h` | Add `TOKEN_CHAR`, `TOKEN_SHORT`, `TOKEN_LONG`. |
| `src/lexer.c` | Map the three new keyword spellings. |
| `include/ast.h` | Add `TypeKind decl_type` to `ASTNode`. |
| `src/parser.c` | Parse `<integer-type>` in declarations, record type. |
| `src/ast_pretty_printer.c` | Print the declared integer type. |
| `test/print_ast/test_variable_declaration.expected` | Format update. |
| `test/print_ast/test_stage_11_01_int_types.{c,expected}` (new) | Covers all four types. |
| `test/valid/test_char_decl__42.c`, `test_short_decl__42.c`, `test_long_decl__42.c` (new) | Exit-code tests exercising codegen with non-int keywords. |
| `docs/grammar.md` | Add `<integer-type>`, update `<declaration>`. |
| `docs/milestones/stage-11-01-summary.md` (new) | Milestone summary. |
| `docs/sessions/stage-11-01-transcript.md` (new) | Session transcript. |

## Implementation Order

1. Types module (`type.h` / `type.c`) + CMake wiring.
2. Tokenizer keywords.
3. AST field + parser updates.
4. AST printer update + expected-file update.
5. New tests (print_ast + valid).
6. Build + full test run; fix regressions if any.
7. Grammar doc update.
8. Milestone + transcript + commit.
