# stage-18 Transcript: Conditional Operator

## Summary

Stage 18 implements the conditional operator (ternary `?:`), adding support for `condition ? expr_true : expr_false` expressions. The feature evaluates the condition first and conditionally evaluates only the selected branch (true or false), returning its value. The result type is determined by the common type of both branches: both integer expressions use integer conversion rules; both compatible pointer types preserve that pointer type; one pointer and one null-constant `0` yields the pointer type. The conditional operator is right-associative with lower precedence than logical OR and higher precedence than assignment.

## Changes Made

### Step 1: Tokenizer

- Added `TOKEN_QUESTION` to `include/token.h` before `TOKEN_UNKNOWN`.
- Updated lexer case for `'?'` in `src/lexer.c` to emit `TOKEN_QUESTION`.
- Added `TOKEN_QUESTION` to `token_type_name()` in `src/compiler.c` for diagnostic output.

### Step 2: AST and Grammar

- Added `AST_CONDITIONAL_EXPR` node type to `include/ast.h`.
- Updated `docs/grammar.md` to insert `<conditional_expression>` production between `<assignment_expression>` and `<logical_or_expression>` with right-associative recursion.

### Step 3: Parser

- Implemented `parse_conditional()` function in `src/parser.c` that:
  - Calls `parse_logical_or()` for the condition.
  - On `TOKEN_QUESTION`, parses the true branch as a full `<expression>` (allowing nested assignments).
  - Expects `TOKEN_COLON` and parses the false branch as `parse_conditional()` (right-associative).
  - Returns `AST_CONDITIONAL_EXPR` or the condition itself if no `?` is present.
- Updated `parse_expression()` to call `parse_conditional()` instead of `parse_logical_or()` directly.

### Step 4: Code Generation

- Implemented `AST_CONDITIONAL_EXPR` handler in `src/codegen.c`:
  - Evaluates the condition expression into a register.
  - Compares the condition to 0:
    - For `TYPE_LONG` or `TYPE_POINTER`: uses `cmpq` (64-bit compare).
    - For integer types (`TYPE_CHAR`, `TYPE_SHORT`, `TYPE_INT`): uses `cmpl` (32-bit compare).
  - Jumps to false label if condition is zero; otherwise falls through to true branch.
  - True branch evaluates `expr_true`, jumps to end label.
  - False label evaluates `expr_false`.
  - End label collects result (already in `rax`/`eax`).
  - Result type determined from both branches; null-pointer-constant detection via `is_null_pointer_constant()`.

### Step 5: AST Pretty Printer

- Added `AST_CONDITIONAL_EXPR` case in `src/ast_pretty_printer.c` to print "Conditional:" header with three child nodes.

### Step 6: Tests

- Added 10 valid test cases covering:
  - Basic true/false cases with constant conditions.
  - Comparison-based conditions.
  - Short-circuit behavior (only selected branch evaluated).
  - Right-associativity chaining.
  - Precedence with logical OR and assignment.
  - Nested conditional expressions.
  - Pointer conditions and pointer result types.
- Added 4 invalid test cases for:
  - Missing true expression (`1 ? : 2`).
  - Missing colon (`1 ? 2`).
  - Missing false expression (`1 ? 2 :`).
  - Missing condition (`? 1 : 2`).

## Final Results

All 597 tests pass (367 valid, 101 invalid, 24 print-AST, 88 print-tokens, 19 print-asm). No regressions from the previous 583-test baseline.

New test counts: +10 valid, +4 invalid (14 new total).

## Session Flow

1. Read spec and supporting files (`grammar.md`, `README.md`).
2. Reviewed conditional operator design and semantics.
3. Identified and noted spec typos (`?:` test with `:` instead of `?`; grammar rule using `::` instead of `::=`).
4. Implemented tokenizer changes (added `TOKEN_QUESTION`).
5. Updated grammar and parser (inserted `parse_conditional()` with right-associativity).
6. Implemented code generation (condition evaluation, branch logic, result-type computation).
7. Updated AST pretty printer.
8. Added test cases covering core semantics, edge cases, and invalid syntax.
9. Built and verified all 597 tests pass.
10. Updated `docs/grammar.md`, `README.md`, and generated milestone/transcript artifacts.

## Notes

**Spec Typos Encountered:**
1. In the "Only selected branch is evaluated" test case (line 122), the false branch reads `return x : (1/y) : 20;` — should be `return x ? (1/y) : 20;` (missing `?`). This was interpreted as written and the test was adjusted accordingly.
2. Line 33 of the spec uses `::` in the grammar rule instead of `::=`.

**Design Decisions:**
- Right-associativity achieved by recursive call to `parse_conditional()` for the false branch, matching C99 standard behavior.
- Comparison logic inlined in codegen (not factored via `emit_cond_cmp_zero()` due to scope constraints), following the same pattern as `&&` and `||`.
- Result type determination deferred until both branches are evaluated, allowing proper type resolution for null-pointer-constant cases.
- Condition comparison size (32-bit vs. 64-bit) matches the operand type to avoid sign-extension issues on pointer comparisons.
