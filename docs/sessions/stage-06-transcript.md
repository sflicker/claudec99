# Stage 6 Transcript: While Statement

## Summary

Extended the ClaudeC99 compiler to support `while` loops, as defined in
`docs/stages/ClaudeC99-spec-stage-06-while-statement.md`.

## Changes Made

### Step 1: Tokenizer

- Added `TOKEN_WHILE` to the `TokenType` enum (between `TOKEN_ELSE` and `TOKEN_IDENTIFIER`)
- Added keyword recognition for `"while"` in `lexer_next_token()`, returning `TOKEN_WHILE`
- Updated the stage comment at the top of the file from Stage 5 to Stage 6

All 39 existing tests passed after this change.

### Step 2: AST + Parser

- Added `AST_WHILE_STATEMENT` to the `ASTNodeType` enum
- Added `parse_while_statement()` function that parses `while ( <expression> ) <statement>`
  - Stores the condition expression as `children[0]`
  - Stores the body statement as `children[1]`
- Wired `TOKEN_WHILE` into `parse_statement()` to dispatch to `parse_while_statement()`

Build succeeded. The compiler accepted while syntax but codegen silently skipped it (no handler yet).

### Step 3: Code Generation

Added `AST_WHILE_STATEMENT` handling in `codegen_statement()`:

```nasm
.L_while_start_N:          ; loop start label
    <evaluate condition>
    cmp eax, 0
    je .L_while_end_N      ; if false, exit loop
    <emit body>
    jmp .L_while_start_N   ; jump back to condition
.L_while_end_N:            ; loop exit
```

Uses the same `label_count` mechanism as `if` statements for unique label IDs.

All 40 tests passed (39 existing + 1 pre-existing while test).

### Step 4: Additional Tests

Added 4 new while loop tests to cover gaps:

| Test | Scenario |
|------|----------|
| `test_while_false_initially__42` | Condition is false from the start — body never executes |
| `test_while_single_stmt__42` | Body is a single statement without braces |
| `test_while_nested__42` | Nested while loops (6 x 7 = 42 iterations) |
| `test_while_countdown__42` | Counts down using `>` (exercises a different comparison operator) |

All 44 tests passed (40 valid + 4 new while tests), plus 2 invalid tests.

## Files Modified

- `src/compiler.c` — tokenizer, AST, parser, and codegen updates for while loops

## Files Added

- `test/valid/test_while_false_initially__42.c`
- `test/valid/test_while_single_stmt__42.c`
- `test/valid/test_while_nested__42.c`
- `test/valid/test_while_countdown__42.c`

## Test Results

```
Results: 44 passed, 0 failed, 44 total (valid)
Results: 2 passed, 0 failed, 2 total (invalid)
```
