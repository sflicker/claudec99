# stage-09-04 Transcript: Function Calls

## Summary

Stage 9.4 adds function call expressions to the compiler. A call may appear
anywhere an expression is allowed, must match an already-defined function
by name and argument count, and returns an `int` in `eax`/`rax`. Arguments
are passed through the SysV AMD64 integer registers `rdi, rsi, rdx, rcx,
r8, r9`, capped at six per call. Argument expressions are evaluated
left-to-right.

The parser now peeks for `(` after an identifier in `parse_primary` to
distinguish a variable reference from a function call, and keeps a
`(name, param_count)` table that is populated at function definition —
the signature is registered before the body is parsed so self-recursion
resolves. The code generator tracks unbalanced `push rax`es in a
`push_depth` counter; at each call site it conditionally emits a
`sub rsp, 8` / `add rsp, 8` pair to keep `rsp` 16-byte aligned at the
`call` instruction. The non-main return epilogue was fixed to
`mov rsp, rbp; pop rbp; ret` when the function has a frame — stage-9.3's
bare `ret` was latent-buggy and only became observable once real calls
happened.

## Plan

Order: AST → parser → codegen → tests → grammar doc → milestone / transcript →
commit. Paused after the kickoff for plan confirmation; ran the full test
suite after each major change.

## Changes Made

### Step 1: AST

- Added `AST_FUNCTION_CALL` to `ASTNodeType` in `include/ast.h`.
- Added a `FunctionCall: <name>` case to `ast_pretty_print` in
  `src/ast_pretty_printer.c`.

### Step 2: Parser

- Added `FuncSig` struct and `funcs[] / func_count` fields to `Parser` in
  `include/parser.h`.
- Added `parser_find_function` / `parser_register_function` helpers in
  `src/parser.c`.
- `parse_primary` now produces `AST_FUNCTION_CALL` when a
  `TOKEN_IDENTIFIER` is followed by `TOKEN_LPAREN`, parsing an optional
  comma-separated `<argument-expression-list>` of `<assignment_expression>`.
- Call-site validation rejects undefined callees, arg-count mismatches,
  and calls with more than 6 arguments.
- `parse_function_decl` registers the defining function's signature
  before the body is parsed so self-calls resolve.

### Step 3: Code Generator

- Added `push_depth` and `has_frame` to `CodeGen` in `include/codegen.h`.
- `codegen_init` zero-initializes both fields.
- In `codegen_function`, `push_depth` is reset and `has_frame` is set to
  `(total_slots > 0)` right before the prologue is emitted.
- Binary op LHS push/pop update `push_depth` by ±1.
- `AST_FUNCTION_CALL` codegen:

```asm
; evaluate and push args (left to right)
; pop into rdi, rsi, rdx, rcx, r8, r9 in reverse order
; if push_depth is odd:
sub rsp, 8
call <name>
add rsp, 8
; else:
call <name>
```

- `AST_RETURN_STATEMENT` for non-main now emits the epilogue when a frame
  exists:

```asm
mov rsp, rbp
pop rbp
ret
```

### Step 4: Tests

- `test/valid/test_call_no_args__42.c`
- `test/valid/test_call_two_args__42.c`
- `test/valid/test_call_six_args__42.c`
- `test/valid/test_call_in_binop__42.c`
- `test/valid/test_call_nested__42.c`
- `test/valid/test_call_recursive_sum__45.c`
- `test/valid/test_call_in_if_cond__42.c`
- `test/valid/test_call_in_declaration_init__42.c`
- `test/invalid/test_invalid_7__call_to_undefined_function.c`
- `test/invalid/test_invalid_8__expects_2_arguments.c`
- `test/invalid/test_invalid_9__call_to_undefined_function.c` (forward ref)
- `test/print_ast/test_function_call.c` and `.expected`
- `test/valid/run_tests.sh` — linker now invoked with `-e main`.

### Step 5: Documentation

- `docs/grammar.md` synced: `<primary_expression>` gains the
  `<function_call>` arm; new `<function_call>` and
  `<argument-expression-list>` productions added.

## Final Results

All 103 tests pass (86 valid + 9 invalid + 8 print_ast). No regressions
from stage 9.3 (previous total: 91). Eight new valid tests, three new
invalid tests, one new print_ast snapshot.

## Session Flow

1. Read the spec and stage 9.3 kickoff / milestone / transcript.
2. Reviewed the tokenizer, parser, AST, codegen, and existing tests.
3. Produced kickoff and planned-changes summary; paused for confirmation.
4. Implemented AST + pretty-printer changes.
5. Implemented parser changes: call parsing, signature table,
   self-call registration, validation errors.
6. Implemented codegen changes: `push_depth` / `has_frame`, call emission
   with alignment pad, non-main epilogue fix.
7. Added valid, invalid, and print_ast tests. Found that the linker's
   default entry is the first symbol in `.text` — updated the valid test
   runner to pass `-e main`.
8. Synced `docs/grammar.md`, wrote the milestone and this transcript,
   committed.

## Notes

- **Alignment math.** Post-prologue `rsp` is 16-aligned (stack size is
  rounded to a multiple of 16). Each unbalanced `push rax` in a binop's
  RHS evaluation flips parity; call-argument pushes balance before the
  `call`. So the parity at the call instruction equals the parity of
  `push_depth` from enclosing binops. An odd value triggers a
  `sub rsp, 8` / `add rsp, 8` pair.
- **Stage 9.3 latent bug.** The bare `ret` after `push rbp; sub rsp, N`
  was never observable because no defined helper was ever called. Stage
  9.4 forces the fix.
- **Entry symbol.** `ld -e main` makes the test harness robust to callee
  definitions that precede `main`, which is the common pattern once
  forward references are disallowed.
