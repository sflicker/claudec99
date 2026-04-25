# Stage-12-03 Milestone Summary: Assignment Through Pointer

Stage 12-03 makes the dereference operator a valid assignment lvalue.
`*p = value` now stores `value` at the address held by `p`, with the
store width chosen from the pointed-to base type.

## Completed
- Parser: extended `parse_expression` so an `=` may follow an
  arbitrary unary/logical-or LHS; the LHS must be `AST_DEREF` or
  `AST_VAR_REF`. Non-lvalue LHS such as `(x + 1) = 3` is rejected
  with "assignment target must be an lvalue".
- AST: no new node types. The existing `AST_ASSIGNMENT` is reused
  with a new shape for deref-LHS — `value` is empty and the two
  children are `[AST_DEREF, RHS]`. Variable-assignment shape is
  unchanged.
- Codegen: in `AST_ASSIGNMENT`, when the LHS is `AST_DEREF`, emit
  the pointer-store path: evaluate the pointer to an address in
  `rax`, push it, evaluate the RHS, convert it to the pointed-to
  base type, pop the address into `rbx`, and emit
  `mov [rbx], al/ax/eax/rax` based on base size. The
  `cannot dereference non-pointer value` check is reused so
  `*x = 3;` (with `x` a non-pointer) is rejected.
- Pretty printer: `AST_ASSIGNMENT` with no name renders as
  `Assignment:` and lets the LHS child print itself.
- Grammar: `<assignment_expression>` now includes
  `<unary_expression> "=" <assignment_expression>` (lvalue
  restriction enforced semantically).

## Tests
- 4 new valid tests: write through pointer, write-then-read-through-
  pointer, nested dereference write (`**pp = 8`), and multiple writes
  through pointer.
- 2 new invalid tests: dereference of non-pointer (`*x = 3`) and
  assignment to a non-lvalue (`(x + 1) = 3`).
- All 230 tests pass (197 valid + 18 invalid + 15 print_ast). No
  regressions.
