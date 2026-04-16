# Stage 7.3 Transcript: Increment and Decrement Operators

## Summary

Extended the ClaudeC99 compiler to support prefix (`++x`, `--x`) and
postfix (`x++`, `x--`) increment and decrement operators for integer
variables, as defined in
`docs/stages/ClaudeC99-spec-stage-07-03-increment-and-decrement.md`.

Prefix operators update the variable and return the new value. Postfix
operators return the old value and then update the variable. Only
identifiers are valid operands — no pointer or complex lvalue semantics.

## Changes Made

### Step 1: Tokenizer

- Added `TOKEN_INCREMENT` and `TOKEN_DECREMENT` to the `TokenType` enum
- Updated the `+` lexer branch to check for `++` before `+=`
- Updated the `-` lexer branch to check for `--` before `-=`

### Step 2: AST + Parser

Added two new AST node types:

- `AST_PREFIX_INC_DEC` — stores operator (`++`/`--`) in `value`, child
  is an `AST_VAR_REF`
- `AST_POSTFIX_INC_DEC` — same layout

Added `parse_postfix()` which parses:

```ebnf
<postfix_expression> ::= <primary_expression> { "++" | "--" }
```

The function calls `parse_primary()`, then loops consuming any trailing
`++` or `--` tokens. It validates that the operand is `AST_VAR_REF`,
erroring otherwise.

Updated `parse_unary()` to handle prefix `++`/`--`:

```ebnf
<unary_expr> ::= [ "+" | "-" | "!" ] <unary_expr>
               | "++" <identifier>
               | "--" <identifier>
               | <postfix_expression>
```

The prefix case consumes the `++`/`--` token, recursively calls
`parse_unary()`, validates the result is `AST_VAR_REF`, and wraps it in
`AST_PREFIX_INC_DEC`. The fallthrough now calls `parse_postfix()` instead
of `parse_primary()`, ensuring postfix binds tighter than prefix.

### Step 3: Code Generator

Added handling for both node types in `codegen_expression()`:

**Prefix (`++x` / `--x`)**:
```asm
    mov eax, [rbp - offset]    ; load variable
    add eax, 1                 ; (or sub eax, 1)
    mov [rbp - offset], eax    ; store new value
    ; eax = new value (returned)
```

**Postfix (`x++` / `x--`)**:
```asm
    mov eax, [rbp - offset]    ; load variable (old value)
    mov ecx, eax               ; copy to ecx
    add ecx, 1                 ; (or sub ecx, 1)
    mov [rbp - offset], ecx    ; store new value
    ; eax = old value (returned)
```

### Step 4: Tests

5 tests already existed from the spec phase:

| Test | Description |
|------|-------------|
| `test_postfix_inc__42` | `a=41; a++; return a;` |
| `test_postfix_dec__42` | `a=43; a--; return a;` |
| `test_prefix_inc__42` | `a=41; ++a; return a;` |
| `test_prefix_dec__42` | `a=43; --a; return a;` |
| `test_for_loop_with_inc__55` | `for (i=1;i<=10;i++) sum+=i;` |

Added 7 new tests for value-returning semantics:

| Test | What it verifies |
|------|------------------|
| `test_postfix_inc_value__41` | `x = a++` returns old value (41, not 42) |
| `test_prefix_inc_value__42` | `x = ++a` returns new value (42) |
| `test_postfix_dec_value__43` | `x = a--` returns old value (43, not 42) |
| `test_prefix_dec_value__42` | `x = --a` returns new value (42) |
| `test_postfix_in_expr__52` | `b = a++ + 42` uses old value of a (10+42=52) |
| `test_prefix_in_expr__42` | `z = --i * 2` uses new value (21*2=42) |
| `test_return_postfix_inc__10` | `return i++` returns old value (10) |

## Final Results

All 69 tests pass (62 existing + 7 new). No regressions.

## Session Flow

1. Read spec, skill notes, and kickoff template
2. Explored existing codebase (`src/compiler.c`, test files)
3. Produced summary and implementation plan
4. **Step 1 — Tokenizer**: added `TOKEN_INCREMENT`/`TOKEN_DECREMENT`,
   updated lexer branches. Built successfully.
5. **Step 2 — AST + Parser**: added `AST_PREFIX_INC_DEC`/`AST_POSTFIX_INC_DEC`,
   added `parse_postfix()`, updated `parse_unary()`. Built successfully.
6. **Step 3 — Code generator**: added codegen for both node types.
   Built and ran all tests — 62/62 passed.
7. **Step 4 — Additional tests**: added 7 tests for value-returning
   semantics. All 69/69 passed.
8. Committed as `db071f3`.
