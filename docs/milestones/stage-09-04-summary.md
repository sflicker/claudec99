# stage-09-04 Milestone: Function Calls

## Overview

Stage 9.4 extends the compiler with function call expressions. A call may
appear anywhere an expression is allowed, evaluates to the `int` value
returned by the callee, and must match an existing function definition by
name and argument count. Arguments are `int`, evaluated left-to-right, and
capped at six so SysV AMD64 register passing is sufficient. Prototypes /
forward declarations remain out of scope; the called function must already
be defined textually above the call site, except for self-recursion where
the function's signature is available inside its own body.

## Grammar Change

Before (stage 9.3):

```ebnf
<primary_expression> ::= <int_literal>
                       | <identifier>
                       | "(" <expression> ")"
```

After (stage 9.4):

```ebnf
<primary_expression> ::= <int_literal>
                       | <identifier>
                       | <function_call>
                       | "(" <expression> ")"

<function_call> ::= <identifier> "(" [ <argument-expression-list> ] ")"

<argument-expression-list> ::= <assignment_expression> { "," <assignment_expression> }
```

`docs/grammar.md` was synced.

## Semantic Rules Enforced

- The called name must be a previously defined function (parser-level).
- Argument count must equal the callee's parameter count (parser-level).
- Maximum six arguments per call (parser-level); matches the stage-9.3
  parameter cap.
- Self-recursion is allowed — the function's signature is registered
  before its body is parsed.
- Forward references are rejected.

## Code Changes

- `include/ast.h` — added `AST_FUNCTION_CALL`.
- `src/ast_pretty_printer.c` — `AST_FUNCTION_CALL` prints as
  `FunctionCall: <name>`.
- `include/parser.h`
  - Added `FuncSig { char name[256]; int param_count; }`.
  - Added `funcs[]` / `func_count` fields on `Parser`.
- `src/parser.c`
  - Added `parser_find_function` and `parser_register_function`.
  - `parse_primary` now detects `<identifier> "("` and parses a
    `<function_call>`; otherwise falls through to `AST_VAR_REF`.
  - `parse_function_decl` registers the signature before parsing the body
    so self-calls resolve.
  - Call validation errors:
    - `error: call to undefined function '<name>'`
    - `error: function '<name>' expects N arguments, got M`
    - `error: function '<name>' call has N arguments; max supported is 6`
- `include/codegen.h` — added `push_depth` and `has_frame` to `CodeGen`.
- `src/codegen.c`
  - `push_depth` is incremented on the binop LHS `push rax` and on each
    call-argument `push rax`; decremented on the matching `pop`.
  - Added `AST_FUNCTION_CALL` emission:
    1. Evaluate each argument into `eax` and push, left-to-right.
    2. Pop into `rdi, rsi, rdx, rcx, r8, r9` in reverse order.
    3. If `push_depth` is odd at the call, wrap the call in
       `sub rsp, 8` / `add rsp, 8`.
    4. Emit `call <name>`.
  - Non-main `return` now emits `mov rsp, rbp; pop rbp; ret` when the
    function has a frame (stage-9.3 bug: the epilogue was missing because
    no defined helper was ever called).
- `test/valid/run_tests.sh` — `ld -e main` sets the explicit entry point so
  helper definitions that precede `main` do not become the default entry.

## AST Shape

A function call node has this layout:

```
FunctionCall: <name>
  <arg-expression-1>
  <arg-expression-2>
  ...
```

Zero-argument calls have no children.

## Stack-Alignment Approach

SysV AMD64 requires `rsp % 16 == 0` at the `call` instruction. Post-prologue
`rsp` is always 16-aligned (stack size is rounded up to a multiple of 16).
Each live `push rax` in an in-progress binop shifts alignment by 8. The
codegen tracks this shift statically in `push_depth`: if it is odd at the
call site, an extra `sub rsp, 8` / `add rsp, 8` pair is emitted. Argument
pushes/pops balance out before the call, so they do not affect the
parity seen at the call instruction.

## Test Changes

### Valid (`test/valid/`)
- `test_call_no_args__42.c` — `main` calls a zero-arg helper.
- `test_call_two_args__42.c` — `add(40, 2)`.
- `test_call_six_args__42.c` — six-argument call (SysV register max).
- `test_call_in_binop__42.c` — call result used in a `+` expression.
- `test_call_nested__42.c` — call whose arguments are themselves calls.
- `test_call_recursive_sum__45.c` — recursive `sum(n)` using `if`.
- `test_call_in_if_cond__42.c` — call used as an `if` condition.
- `test_call_in_declaration_init__42.c` — call used as an initializer.

### Invalid (`test/invalid/`)
- `test_invalid_7__call_to_undefined_function.c` — unknown name.
- `test_invalid_8__expects_2_arguments.c` — arg-count mismatch.
- `test_invalid_9__call_to_undefined_function.c` — forward reference to a
  function defined later in the translation unit.

### Print-AST (`test/print_ast/`)
- `test_function_call.c` / `.expected` — snapshot containing a
  `FunctionCall: add` under a `ReturnStmt:`.

## Test Results

- `test/valid` — 86 / 86 pass (78 prior + 8 new).
- `test/invalid` — 9 / 9 pass (6 prior + 3 new).
- `test/print_ast` — 8 / 8 pass (7 prior + 1 new).
- Total: 103 / 103. No regressions.

## Constraints Honored

- Identifier-only callee; no `(foo)(1)` and no function pointers.
- Max 6 arguments (parser rejects more).
- Int-only return / parameter / argument types.
- No prototypes / forward declarations.
- Forward-reference calls rejected by the parser.

## Decisions & Rationale

- **Parser-level call validation.** The parser already maintains a
  function-name set for duplicate detection; extending it with parameter
  counts keeps all semantic checks in one place and guarantees the AST
  handed to codegen is valid.
- **Register signature before body.** Self-recursion requires the function
  to be in scope inside its own body. Registering the signature right
  before parsing the body is the minimal change that satisfies this.
- **Static alignment tracking.** Rather than dynamically measuring `rsp`
  with `and rsp, -16` and saving/restoring, the compiler tracks
  `push_depth` during codegen and conditionally emits the pad. This is
  simpler and emits tighter code when no pad is needed.
- **Non-main epilogue fix.** Stage 9.3 emitted a bare `ret` after a
  `push rbp / sub rsp, N` prologue. That was latent-buggy because no
  defined helper was ever called; as soon as stage 9.4 issues a real
  `call`, the bug becomes observable as a crash on return. The epilogue
  is now emitted when the function has a frame.
- **`ld -e main` in the valid test runner.** Previously tests relied on
  `main` being the first symbol in `.text` so the linker's default entry
  (start of `.text`) happened to be `main`. Stage 9.4 tests naturally
  define the callee before `main`, so the runner now sets the entry
  symbol explicitly.
