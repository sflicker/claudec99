# Stage-12-03 Kickoff: Assignment Through Pointer

## Spec Summary
Stage 12-03 extends the compiler to support **assignment through a
pointer**: `*p = value`. The dereference operator (added in 12-02 as
an rvalue) is now also a valid assignment lvalue when its operand has
pointer type. The compiler must emit a size-appropriate memory store
through the address in `p`, and the parser must reject non-lvalue
assignment targets such as `(x + 1) = 3`.

## What changes from Stage 12-02
Stage 12-02 introduced the unary `&` and `*` operators with `*p` only
in rvalue (load) position. This stage adds `*p` as an lvalue
(assignment target). No new tokens or AST node types are needed; the
existing `AST_ASSIGNMENT` and `AST_DEREF` are reused. The parser
gains the ability to recognize a dereference expression on the
left of `=`, and codegen gains a pointer-store path that uses the
address-not-the-load semantics of `AST_DEREF` in lvalue position.

## Spec issues / clarifications
1. **Bug in the "Nested dereference write" example**: the spec writes
   `**p = 8;` where `p` is `int *`, so `*p` is `int` and `**p` is a
   type error. With `int **pp = &p;` declared, the intent is
   `**pp = 8;`. Implementation uses `**pp = 8;`.
2. **Missing `}`** at end of the "Nested dereference" snippet — typo;
   the test closes the function block correctly.
3. Minor typo: "evalulate" in the codegen-notes block.
4. The spec says "Grammar updates: None". This is true at the level
   of the published grammar, but the implementation must extend the
   parser's `<assignment_expression>` handling to recognize a
   dereference on the LHS. The published grammar in `docs/grammar.md`
   will be updated to reflect that the assignment LHS may also be a
   `<unary_expression>` (with the lvalue restriction enforced
   semantically), which matches existing convention for similar
   restrictions (e.g. prefix `++/--`).
5. The "Invalid: dereference non-pointer" case is caught at codegen
   time, reusing the stage 12-02 "cannot dereference non-pointer"
   error. The "Invalid: non-lvalue assignment" case is caught at
   parse time.

## Planned Changes

1. **Tokenizer** — no changes.
2. **AST** — no changes. `AST_ASSIGNMENT` is reused with a new shape
   for deref assignment: `value` is empty/NULL and two children
   `[AST_DEREF, RHS]`. The existing variable assignment shape
   (`value=name`, one RHS child) is preserved.
3. **Parser** (`src/parser.c`)
   - Extend `parse_expression`. After the existing `<identifier> "="`
     / `"+=" / "-="` shortcut, parse a `parse_logical_or` LHS and, if
     the next token is `=`, build an `AST_ASSIGNMENT` with children
     `[LHS, RHS]`. Reject LHS that is not `AST_DEREF` (or
     `AST_VAR_REF`, defensively, although the shortcut already covers
     bare-identifier lvalues). `(x + 1) = 3` is rejected here.
4. **AST pretty printer** (`src/ast_pretty_printer.c`)
   - Render `AST_ASSIGNMENT` with no `value` as `Assignment:` (no
     trailing name); the LHS child prints naturally.
5. **Code generator** (`src/codegen.c`)
   - In `AST_ASSIGNMENT`: if `child_count == 2` and
     `children[0]->type == AST_DEREF`, take the pointer-store path:
     evaluate the pointer expression to get an address in `rax`
     (validating pointer type), `push rax`, evaluate RHS into
     `rax/eax`, convert to the pointed-to base type, `pop rbx`,
     and emit `mov [rbx], al/ax/eax/rax` based on base size.
   - Otherwise keep the existing variable-store path.
6. **Tests**
   - `test/valid/test_write_through_pointer__9.c` — basic
     `*p = 9; return x;` (expect 9).
   - `test/valid/test_write_then_read_through_pointer__5.c` —
     `*p = 5; return *p;` (expect 5).
   - `test/valid/test_nested_dereference_write__8.c` —
     `**pp = 8; return x;` (expect 8).
   - `test/valid/test_multiple_writes_through_pointer__3.c` —
     `*p = 2; *p = 3; return *p;` (expect 3).
   - `test/invalid/test_invalid_17__cannot_dereference.c` —
     `*x = 3;` where `x` is `int`.
   - `test/invalid/test_invalid_18__assignment_to_non_lvalue.c` —
     `(x + 1) = 3;`.
7. **Documentation**
   - Update `docs/grammar.md`: extend `<assignment_expression>` to
     allow `<unary_expression> "=" <assignment_expression>`.
8. **Commit** at the end with a concise message.
