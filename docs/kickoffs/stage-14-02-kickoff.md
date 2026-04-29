# Stage-14-02 Kickoff: String Literal AST Node

## Spec Summary

Stage 14-02 adds parser and AST support for string literals. Stage
14-01 already taught the lexer to emit `TOKEN_STRING_LITERAL` with a
byte payload and byte length; this stage wires that token into the
parser and AST layer.

Concretely:
  - A new AST node kind, `AST_STRING_LITERAL`, is introduced.
  - `<primary_expression>` is extended with a `<string_literal>`
    alternative that consumes a single `TOKEN_STRING_LITERAL`.
  - The literal's body bytes and byte length are preserved on the
    AST node.
  - A string literal's logical type is `char[N]` where
    `N = byte_length + 1` (the trailing NUL).
  - The AST pretty-printer renders the new node.
  - Two `print_ast` tests are added.

Out of scope: emitting `.rodata`, generating addresses for string
literals, assigning to `char *`, escape sequences, char-array
initialization from string literals, adjacent-literal concatenation,
and any runtime tests.

## What Must Change vs. Stage 14-01

  - `include/ast.h`: add `AST_STRING_LITERAL` to `ASTNodeType`.
  - `src/parser.c`: in `parse_primary`, accept `TOKEN_STRING_LITERAL`
    and build the new AST node — copying the byte payload to
    `node->value` and storing length implicitly via
    `full_type = type_array(type_char(), token.length + 1)`. Set
    `decl_type = TYPE_ARRAY` so the pretty-printer can route to the
    right branch.
  - `src/ast_pretty_printer.c`: add a `AST_STRING_LITERAL` case.
  - `test/print_ast/`: two new test programs (basic and empty
    string) with `.expected` files.
  - `docs/grammar.md`: add `<string_literal>` and update
    `<primary_expression>`.

No lexer, codegen, or token changes.

## Decisions

  1. **AST representation of byte length.** `ASTNode` has no `length`
     field. Rather than add one, store length via the existing
     `full_type` slot using
     `type_array(type_char(), byte_length + 1)`. This naturally
     preserves the byte length, matches the spec's "char[N] where N
     is byte length + 1" type rule, and reuses existing
     infrastructure.
  2. **Pretty-printer format.** `StringLiteral: "<bytes>" (length N)`
     with quotes around the body so empty strings render
     unambiguously, and the byte length printed explicitly to make
     the preserved-length property visible. This mirrors the
     existing `IntLiteral: <value>` style while exposing the
     additional state.
  3. **Parser placement.** A new branch at the top of
     `parse_primary` handles `TOKEN_STRING_LITERAL` before the
     `TOKEN_IDENTIFIER` and `TOKEN_LPAREN` branches, parallel to the
     existing `TOKEN_INT_LITERAL` branch.
  4. **Codegen.** No change. The existing `codegen_expression`
     dispatch falls through to a no-op for unknown node types, and
     the tests added this stage exercise only `--print-ast`.
  5. **Grammar.md update.** Add a `<string_literal>` production and
     extend `<primary_expression>`. No other grammar lines change.

## Spec Issues Flagged

  - The spec is silent on the exact pretty-printer format. Decided
    on `StringLiteral: "<bytes>" (length N)` (see Decisions §2).
  - The spec says "Preserve the string literal byte length in the
    AST node" without specifying the storage. Decided to use the
    existing `full_type` slot via a `char[N]` array type rather than
    add a new `length` field to `ASTNode` (see Decisions §1).

## Planned Changes

  1. **Tokenizer** — none.
  2. **Parser** — extend `parse_primary` (in `src/parser.c`) with a
     `TOKEN_STRING_LITERAL` branch that emits an `AST_STRING_LITERAL`
     node, copies the bytes, and sets the `char[N+1]` full_type.
  3. **AST** — add `AST_STRING_LITERAL` to `ASTNodeType` in
     `include/ast.h`.
  4. **Code generator** — none.
  5. **Pretty printer** — add a case for `AST_STRING_LITERAL` in
     `src/ast_pretty_printer.c` that renders
     `StringLiteral: "<bytes>" (length N)`.
  6. **Tests** —
     - `test/print_ast/test_stage_14_02_string_literal_basic.c`
       (and `.expected`) — `return "hi";` inside `int main()`.
     - `test/print_ast/test_stage_14_02_string_literal_empty.c`
       (and `.expected`) — `return "";`.
  7. **Grammar** — update `docs/grammar.md` to add `<string_literal>`
     and the new `<primary_expression>` alternative.
  8. **Docs** — kickoff (this), milestone, session transcript.
  9. **Commit** — single commit when all suites are green.
