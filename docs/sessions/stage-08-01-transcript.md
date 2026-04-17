# Stage 8.1 Transcript: Logical Operators `&&` and `||`

## Summary

Implemented the logical AND (`&&`) and logical OR (`||`) operators as
defined in `docs/stages/ClaudeC99-spec-stage-08-01-logical-operators.md`.

The two operators:
- Produce an integer result: `1` for true, `0` for false.
- Use **short-circuit evaluation**: for `a && b`, `b` is not evaluated if
  `a` is false; for `a || b`, `b` is not evaluated if `a` is true.
- Follow standard C precedence: `||` is lower than `&&`, and both are
  lower than equality (`==`, `!=`) and relational (`<`, `<=`, `>`, `>=`)
  operators.

All changes were made in `src/compiler.c`. No new AST node type was
added — `&&` and `||` reuse `AST_BINARY_OP` and are special-cased in the
code generator because the generic binary-op path would eagerly emit
both operands.

## Changes Made

### Step 1: Tokenizer

Added two new token types to the `TokenType` enum:

```c
TOKEN_AND_AND,
TOKEN_OR_OR,
```

Added two lexer cases in `lexer_next_token`, placed alongside the other
two-character operator tokens:

```c
if (c == '&' && n == '&') { token.type = TOKEN_AND_AND; strcpy(token.value, "&&"); lexer->pos += 2; return token; }
if (c == '|' && n == '|') { token.type = TOKEN_OR_OR;   strcpy(token.value, "||"); lexer->pos += 2; return token; }
```

A lone `&` or `|` is not a valid token at this stage and falls through
to `TOKEN_UNKNOWN`, matching prior-stage style of recognizing only what
the spec requires.

### Step 2: Parser

Added two new recursive-descent functions between `parse_equality` and
`parse_expression`, matching the left-associative chain style already
used by the other precedence levels:

```c
/* <logical_and_expression> ::= <equality_expression> { "&&" <equality_expression> } */
static ASTNode *parse_logical_and(Parser *parser) {
    ASTNode *left = parse_equality(parser);
    while (parser->current.type == TOKEN_AND_AND) {
        Token op = parser->current;
        parser->current = lexer_next_token(parser->lexer);
        ASTNode *right = parse_equality(parser);
        ASTNode *binop = ast_new(AST_BINARY_OP, op.value);
        ast_add_child(binop, left);
        ast_add_child(binop, right);
        left = binop;
    }
    return left;
}

/* <logical_or_expression> ::= <logical_and_expression> { "||" <logical_and_expression> } */
static ASTNode *parse_logical_or(Parser *parser) {
    ASTNode *left = parse_logical_and(parser);
    while (parser->current.type == TOKEN_OR_OR) {
        Token op = parser->current;
        parser->current = lexer_next_token(parser->lexer);
        ASTNode *right = parse_logical_and(parser);
        ASTNode *binop = ast_new(AST_BINARY_OP, op.value);
        ast_add_child(binop, left);
        ast_add_child(binop, right);
        left = binop;
    }
    return left;
}
```

Updated `parse_expression` so its non-assignment fallthrough calls
`parse_logical_or` (the new lowest-precedence expression) instead of
`parse_equality`. The precedence chain is now:

```
assignment -> logical_or -> logical_and -> equality -> relational
           -> additive   -> multiplicative -> unary -> postfix -> primary
```

### Step 3: AST Pretty-Printer

Extended `operator_name` in the pretty-printer so `--print-ast` emits
readable labels for the new operators, matching the style of existing
entries:

```c
if (strcmp(op, "&&") == 0) return "AND";
if (strcmp(op, "||") == 0) return "OR";
```

No new AST node type, no changes to `ast_pretty_print` switch cases.

### Step 4: Code Generation

The generic `AST_BINARY_OP` path evaluates both operands unconditionally
(left -> `push rax`, right -> `pop rcx`, then apply op). Short-circuit
evaluation forbids this, so `&&` and `||` are handled at the top of the
`AST_BINARY_OP` branch in `codegen_expression`, before the generic
evaluation order:

**`&&`** — if left is 0, skip RHS and produce 0; otherwise normalize
`right != 0` to 0/1:

```
<codegen left into eax>
cmp eax, 0
je .L_and_false_N
<codegen right into eax>
cmp eax, 0
setne al
movzx eax, al
jmp .L_and_end_N
.L_and_false_N:
mov eax, 0
.L_and_end_N:
```

**`||`** — if left is nonzero, skip RHS and produce 1; otherwise
normalize `right != 0` to 0/1:

```
<codegen left into eax>
cmp eax, 0
jne .L_or_true_N
<codegen right into eax>
cmp eax, 0
setne al
movzx eax, al
jmp .L_or_end_N
.L_or_true_N:
mov eax, 1
.L_or_end_N:
```

Labels use the same `.L_..._N` convention already used by `if`, `while`,
and `for` statements, with `cg->label_count` providing the unique `N`.
The `setne` + `movzx` sequence on the RHS path produces a canonical
0/1 integer result, as required by the spec.

### Step 5: Tests

Two tests were present (untracked) at the start of the stage:

| Test File                  | Source                        | Expected |
|----------------------------|-------------------------------|----------|
| `test_logical_and__1.c`   | `return (1 < 2) && (5 == 5);` | 1        |
| `test_logical_or__1.c`    | `return (1 < 2) || (5 == 5);` | 1        |

Both exercise the core precedence rule (`&&` / `||` lower than `<` and
`==`) and the expected 0/1 integer result. No additional tests were
added — the spec does not call for more, and the existing 70 valid
tests cover all neighboring constructs (equality, relational, if/while/
for, assignments).

### What Did NOT Change

- **AST**: no new node types — `AST_BINARY_OP` is reused.
- **Existing codegen paths**: the fallthrough binary-op flow, unary
  operators, control-flow statements, and variable handling are
  unchanged.
- **Parser entry points** other than `parse_expression`: unchanged.
- **CLI** and file I/O: unchanged.

## Test Results

```
-- valid tests --
PASS  test_logical_and__1  (exit code: 1)
PASS  test_logical_or__1   (exit code: 1)
Results: 72 passed, 0 failed, 72 total

-- invalid tests --
Results: 2 passed, 0 failed, 2 total

-- print_ast tests --
Results: 6 passed, 0 failed, 6 total
```

No regressions in any of the three test suites.

## Spec Notes

One minor stylistic carryover (not blocking, not acted on): line 80 of
the spec reads `<int_literal> ::= [0-9]+c` with a trailing `c` that
appears to be a stray character (it is present in prior stage specs
too, so I treated it as a harmless holdover rather than a change to
the grammar).
