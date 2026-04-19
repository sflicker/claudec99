# stage-09-04 Kickoff: Function Calls

## Spec Summary

Stage 9.4 introduces function call expressions. A call may appear anywhere
an expression is allowed, evaluates to the `int` value returned by the
callee, and must match an existing function definition by name and argument
count. Arguments are `int` and are evaluated left-to-right. The stage caps
call arguments at six so that SysV AMD64 register passing
(`rdi, rsi, rdx, rcx, r8, r9`) is sufficient. The return value arrives in
`rax`.

Function declarations / prototypes are still out of scope. Function call
expressions are identifier-only — `(foo)(1)` and function pointers are
rejected by the grammar. A function may call itself, but forward references
are not allowed: the called function must already be defined textually.

## Semantic Rules (from spec)

- Call name must match a previously defined function.
- Call argument count must match the definition's parameter count.
- All functions return `int`; all parameters and arguments are `int`.
- Arguments are evaluated left-to-right.
- Max six arguments per call (matches stage 9.3 parameter cap).
- Self-recursion is permitted — a function is in scope inside its own body.
- Calling convention is SysV AMD64: first six int args in
  `rdi/rsi/rdx/rcx/r8/r9`, return in `rax`.

## Changes from Stage 9.3

### AST
- Add `AST_FUNCTION_CALL`. The node's `value` is the function name and
  its children are the argument expressions (zero or more).

### Pretty Printer
- Print `FunctionCall: <name>` for `AST_FUNCTION_CALL`.

### Parser
- In `parse_primary`, after consuming a `TOKEN_IDENTIFIER`, peek for
  `TOKEN_LPAREN`. If present, parse an `<argument-expression-list>` as a
  function call; otherwise produce `AST_VAR_REF`.
- Maintain a parser-side table of `(name, param_count)` for every function
  definition.
- Add the defining function's signature to the table *before* parsing its
  body so self-calls resolve.
- Report `call to undefined function '<name>'` when a call's name is not
  in the table.
- Report `function '<name>' expects N arguments, got M` when arg count
  does not match the recorded param count.

### Code Generation
- Add `int push_depth` to `CodeGen` to track how many unbalanced
  `push rax`es currently live on the stack during an expression.
  - Increment on the `push rax` for a binary op's LHS.
  - Decrement on the matching `pop rcx`.
  - Increment on each call-argument push; decrement on each pop into an
    argument register.
- Add `int has_frame` to `CodeGen` so returns know whether to emit the
  `mov rsp, rbp; pop rbp` epilogue.
- Implement `AST_FUNCTION_CALL`:
  1. Evaluate each argument into `eax` left-to-right, pushing onto the
     stack (incrementing `push_depth`).
  2. Pop arguments in reverse order into `rdi, rsi, rdx, rcx, r8, r9`
     (decrementing `push_depth`).
  3. If `push_depth` is odd at the call site, emit
     `sub rsp, 8` / `call foo` / `add rsp, 8`; otherwise just `call foo`.
  4. Result stays in `eax`/`rax`.
- Fix non-main return epilogue: when the function has a frame, emit
  `mov rsp, rbp; pop rbp; ret` instead of a bare `ret`. (Stage 9.3 was
  latent-buggy here because defined helpers were never called.)

### Documentation
- Sync `docs/grammar.md` to the stage-9.4 grammar (adds `<function_call>`
  and `<argument-expression-list>`, and the `<function_call>` arm of
  `<primary_expression>`).

## Ambiguities / Decisions

1. **Forward references.** Spec says "the called function must already be
   defined." Reading: textual / top-down order. Registering the function's
   signature *before* parsing its body is sufficient for self-calls while
   still rejecting forward calls to functions defined later.
2. **Validation location.** Argument count / undefined name checks are
   done in the parser (which already tracks function names for
   duplicate-definition detection) rather than in codegen. The AST handed
   to codegen is already valid with respect to these rules.
3. **Stack alignment.** SysV AMD64 requires `rsp % 16 == 0` at the call
   instruction. Since binop evaluation pushes `rax` during RHS evaluation
   (making rsp 8-off), a statically-tracked `push_depth` counter tells us
   at each call site whether a pad is needed. Odd depth → `sub rsp, 8` /
   `add rsp, 8` around the call.
4. **Non-main epilogue.** Stage 9.3's non-main return emitted bare `ret`
   after a `push rbp / sub rsp, X` prologue; that was never exercised
   because helpers were not called. Stage 9.4 must fix this.
5. **Spec typo.** "Stagea 09-04" in the spec title — cosmetic only.

## Planned File Updates

| File | Change |
|---|---|
| `include/ast.h` | `AST_FUNCTION_CALL` |
| `src/ast_pretty_printer.c` | `FunctionCall:` label |
| `src/parser.c` | Call path in `parse_primary`; signature table; call-validation |
| `include/codegen.h` | `push_depth`, `has_frame` |
| `src/codegen.c` | Call emission + alignment; non-main frame epilogue |
| `docs/grammar.md` | Sync grammar |
| `test/valid/*.c` | Call-centric tests (no-arg, one-arg, six-arg, nested, recursive) |
| `test/invalid/*.c` | Undefined call; arg-count mismatch; forward-reference |
| `test/print_ast/*` | Function-call snapshot |

## Test Plan (preview)

- **Valid:**
  - `int helper(){ return 42; } int main(){ return helper(); }`
  - `int add(int a,int b){ return a+b; } int main(){ return add(40,2); }`
  - Six-arg call.
  - Call nested inside a binop, e.g. `return add(1,2) + add(40,-1);`
  - Recursive sum (`int sum(int n){ if(n==0) return 0; return n + sum(n-1); }`) with controlled input.
  - Function call used in an `if` condition and/or variable initializer.
- **Invalid:**
  - Call undefined function name.
  - Arg-count mismatch (call with fewer / more args than definition).
  - Forward reference (call a function defined later in the translation unit).
- **print_ast:** Function whose body contains a call with two argument expressions.

## Next Step

Pause for confirmation of the plan above before beginning implementation
with the AST change.
