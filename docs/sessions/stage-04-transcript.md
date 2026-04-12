# Stage 4 Implementation Transcript

## Task
Extend the ClaudeC99 compiler to support unary expressions, block statements, expression statements, and if/else control flow, as defined in `ClaudeC99-spec-stage-04-adding-unary-if-else-block.md`.

---

## Step 0: Spec Summary and Change Analysis

### New Tokens
- `if` keyword
- `else` keyword
- `!` (logical NOT, standalone — distinct from `!=`)
- `identifier` (already existed from stage 3)

### Grammar Changes
1. **`<block>`** — new production: `"{" { <statement> } "}"`
2. **`<statement>`** — expanded: `return_statement | if_statement | block | expression_stmt`
3. **`<if_statement>`** — new: `"if" "(" <expression> ")" <statement> [ "else" <statement> ]`
4. **`<expression_stmt>`** — new: `<expression> ";"`
5. **`<unary_expr>`** — new precedence level between `<term>` and `<primary>`: `[ "+" | "-" | "!" ] <unary_expr> | <primary>`
6. **`<function>`** — body becomes a `<block>` instead of a single `<statement>`

### AST Changes
- `AST_UNARY_OP` — operator + child expression
- `AST_IF_STATEMENT` — condition, then-stmt, optional else-stmt
- `AST_BLOCK` — zero or more child statements
- `AST_EXPRESSION_STMT` — wraps an expression used as a statement

### Semantics
- `!` returns 1 if operand is 0, otherwise 0
- Unary `-` negates, unary `+` is identity
- `if` uses integer truthiness (0 = false, nonzero = true)
- `else` binds to nearest unmatched `if`

---

## Step 1: Tokenizer Updates

### What changed and why

| Change | Why |
|--------|-----|
| Added `TOKEN_IF`, `TOKEN_ELSE` to enum | New keywords required by the spec |
| Added `TOKEN_BANG` to enum | Standalone `!` operator for unary NOT |
| Added `if (c == '!')` handler after the `!=` check | Produces `TOKEN_BANG` when `!` is not followed by `=` |
| Added `"if"` and `"else"` keyword matching | Maps identifier strings to their keyword token types |
| Updated stage comment from "Stage 3" to "Stage 4" | Reflects current stage |

### Token enum (after changes)

```c
typedef enum {
    TOKEN_EOF,
    TOKEN_INT,
    TOKEN_RETURN,
    TOKEN_IF,
    TOKEN_ELSE,
    TOKEN_IDENTIFIER,
    TOKEN_INT_LITERAL,
    TOKEN_LPAREN,
    TOKEN_RPAREN,
    TOKEN_LBRACE,
    TOKEN_RBRACE,
    TOKEN_SEMICOLON,
    TOKEN_PLUS,
    TOKEN_MINUS,
    TOKEN_STAR,
    TOKEN_SLASH,
    TOKEN_BANG,
    TOKEN_EQ,
    TOKEN_NE,
    TOKEN_LT,
    TOKEN_LE,
    TOKEN_GT,
    TOKEN_GE,
    TOKEN_UNKNOWN
} TokenType;
```

### Lexer: standalone `!` handling

Added after the `!=` check:
```c
if (c == '!' && n == '=') { token.type = TOKEN_NE; strcpy(token.value, "!="); lexer->pos += 2; return token; }
if (c == '!') { token.type = TOKEN_BANG; token.value[0] = c; lexer->pos++; return token; }
```

### Lexer: keyword matching

Added `if` and `else` to the keyword section:
```c
} else if (strcmp(token.value, "if") == 0) {
    token.type = TOKEN_IF;
} else if (strcmp(token.value, "else") == 0) {
    token.type = TOKEN_ELSE;
}
```

### Verification
- Project compiled cleanly.
- All 21 existing stage 3 tests passed (no regressions).
- 7 new stage 4 test files failed as expected (parser/codegen not yet updated).

---

## Step 2: AST and Parser Updates

### AST changes

| Change | Why |
|--------|-----|
| Added `AST_UNARY_OP`, `AST_IF_STATEMENT`, `AST_BLOCK`, `AST_EXPRESSION_STMT` | New node types required by spec |
| `children[4]` changed to `children[AST_MAX_CHILDREN]` (64) | Blocks need variable number of children |
| `ast_add_child` limit updated to `AST_MAX_CHILDREN` | Matches new array size |

### Parser changes

| Change | Why |
|--------|-----|
| `parse_factor` renamed to `parse_primary` | Matches spec grammar naming |
| New `parse_unary` | Handles `+`, `-`, `!` prefix operators; recurses for chaining |
| `parse_term` calls `parse_unary` instead of `parse_factor` | Unary sits between term and primary in precedence |
| New `parse_block` | Parses `{ stmt* }`, creates `AST_BLOCK` with children |
| New `parse_if_statement` | Parses `if (expr) stmt [else stmt]`, creates `AST_IF_STATEMENT` with 2-3 children |
| `parse_statement` expanded | Dispatches on token: `return` -> return_stmt, `if` -> if_stmt, `{` -> block, else -> expr_stmt |
| `parse_function_decl` uses `parse_block` | Function body is now `<block>` per grammar |

### Key design choices
- `AST_IF_STATEMENT` uses 2 children (no else) or 3 children (with else) — `child_count` distinguishes them.
- `parse_statement` for `return` consumes the `return` token directly, consistent with how `if` and block dispatchers work.
- `else` binds to nearest `if` naturally via recursive descent.

### Verification
- Project compiled cleanly.
- All stage 4 test files parsed without errors (compiler ran without crashing).
- Codegen output was empty for new constructs (expected — codegen not yet updated).

---

## Step 3: Code Generation Updates

### What changed and why

| Change | Why |
|--------|-----|
| Added `label_count` to `CodeGen` struct | Unique labels needed for if/else jump targets |
| `codegen_init` initializes `label_count = 0` | Start label numbering from 0 |
| `AST_UNARY_OP` handling in `codegen_expression` | `-` emits `neg eax`; `!` emits `cmp eax,0; sete al; movzx eax,al`; `+` is a no-op |
| `AST_IF_STATEMENT` handling in `codegen_statement` | Evaluates condition, `je` to else/end label, generates then/else branches |
| `AST_BLOCK` handling in `codegen_statement` | Iterates over children, generates each statement |
| `AST_EXPRESSION_STMT` handling in `codegen_statement` | Evaluates expression (result in eax, discarded) |

### Unary codegen

```c
if (node->type == AST_UNARY_OP) {
    codegen_expression(cg, node->children[0]);
    const char *op = node->value;
    if (strcmp(op, "-") == 0) {
        fprintf(cg->output, "    neg eax\n");
    } else if (strcmp(op, "!") == 0) {
        fprintf(cg->output, "    cmp eax, 0\n");
        fprintf(cg->output, "    sete al\n");
        fprintf(cg->output, "    movzx eax, al\n");
    }
    /* unary + is a no-op */
    return;
}
```

### If/else codegen

```c
} else if (node->type == AST_IF_STATEMENT) {
    int label_id = cg->label_count++;
    codegen_expression(cg, node->children[0]);
    fprintf(cg->output, "    cmp eax, 0\n");
    if (node->child_count == 3) {
        /* if/else */
        fprintf(cg->output, "    je .L_else_%d\n", label_id);
        codegen_statement(cg, node->children[1], is_main);
        fprintf(cg->output, "    jmp .L_end_%d\n", label_id);
        fprintf(cg->output, ".L_else_%d:\n", label_id);
        codegen_statement(cg, node->children[2], is_main);
        fprintf(cg->output, ".L_end_%d:\n", label_id);
    } else {
        /* if without else */
        fprintf(cg->output, "    je .L_end_%d\n", label_id);
        codegen_statement(cg, node->children[1], is_main);
        fprintf(cg->output, ".L_end_%d:\n", label_id);
    }
}
```

### Test file rename

`test_unary_neg__-42.c` was renamed to `test_unary_neg__214.c` because Linux exit codes are unsigned 0-255. The value `-42` wraps to `214` (`-42 & 0xFF`). The test runner extracts the expected value from the filename, so the filename must match the actual exit code.

### Verification

All 28 tests pass:

```
PASS  test_add__42  (exit code: 42)
PASS  test_add_multiply__42  (exit code: 42)
PASS  test_basic_if__42  (exit code: 42)
PASS  test_compound_equality_true__1  (exit code: 1)
PASS  test_divide__42  (exit code: 42)
PASS  test_eq_false__0  (exit code: 0)
PASS  test_eq_true__1  (exit code: 1)
PASS  test_expr_eq_true__1  (exit code: 1)
PASS  test_expr_lt_true__1  (exit code: 1)
PASS  test_ge_false__0  (exit code: 0)
PASS  test_ge_true__1  (exit code: 1)
PASS  test_gt_false__0  (exit code: 0)
PASS  test_gt_true__1  (exit code: 1)
PASS  test_if_else_block__42  (exit code: 42)
PASS  test_if_with_blocks__42  (exit code: 42)
PASS  test_if_with_else_block__42  (exit code: 42)
PASS  test_le_false__0  (exit code: 0)
PASS  test_le_true__1  (exit code: 1)
PASS  test_lt_false__0  (exit code: 0)
PASS  test_lt_true__1  (exit code: 1)
PASS  test_multiply__42  (exit code: 42)
PASS  test_ne_false__0  (exit code: 0)
PASS  test_ne_true__1  (exit code: 1)
PASS  test_paren__42  (exit code: 42)
PASS  test_return__42  (exit code: 42)
PASS  test_unary_bang_nonzero__0  (exit code: 0)
PASS  test_unary_bang_zero__1  (exit code: 1)
PASS  test_unary_neg__214  (exit code: 214)

Results: 28 passed, 0 failed, 28 total
```

---

## Files Modified

- `src/compiler.c` — tokenizer, AST, parser, and codegen updates
- `test/test_unary_neg__-42.c` — renamed to `test/test_unary_neg__214.c`

## Stage 4 Complete

All features from the spec are implemented:
- Unary operators (`+`, `-`, `!`)
- Block statements (`{ ... }`)
- If/else control flow
- Expression statements
- 28/28 tests passing with no regressions
